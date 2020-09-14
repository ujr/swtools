
#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "sorting.h"
#include "strbuf.h"

static jmp_buf bufjump;
static void buf_nomem(void)
{
  longjmp(bufjump, 1);
}

#define BUF_ABORT buf_nomem()
#include "buf.h"

struct lines {
  char *linebuf; // buf.h
  size_t *linepos; // buf.h
  size_t chunksize;
};

static int memsort(struct lines *plines, FILE *fin, FILE *fout);
static int extsort(struct lines *plines, FILE *fin, FILE *fout);
static int compare(const char *s, const char *t);

static size_t appendline(char **buf, FILE *fp);
static int readlines(struct lines *plines, FILE *fp);
static void sortlines(struct lines *plines);
static void writelines(struct lines *plines, FILE *fp);
static void freelines(struct lines *plines);

static void nametemp(char *buf, size_t len, int num);
static FILE *maketemp(int num);
static FILE *opentemp(int num);
static void droptemp(int num);
static void opentemps(FILE **fps, int lo, int hi);
static void droptemps(FILE **fps, int lo, int hi);

static void merge(FILE **infps, int numfp, FILE *outfp);
static void quick(size_t *v, size_t lo, size_t hi, const char *linebuf);

static int parseopts(int argc, char **argv, size_t *chunksize);
static void usage(const char *errmsg);

static bool reverse = false;
static bool casefold = false;
static bool dictsort = false;
static bool numeric = false;

#define MERGEORDER 5
#define PATHBUFLEN 256
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define CHECKIOERR(fp, msg) if (ferror(fp)) { printerr(msg); return FAILSOFT; }
#define CHECKTMPERR(fps, n, msg) do { for (int i=0;i<n;i++) \
  if (ferror(fps[i])) { printerr(msg); return FAILSOFT; }} while(0)

int
sortcmd(int argc, char **argv)
{
  int r;
  FILE *fin;
  struct lines lines = { 0, 0, 0 }; // must zero-init for buf.h

  r = parseopts(argc, argv, &lines.chunksize);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc > 0 && *argv) {
    argc--;
    fin = openin(*argv++);
    if (!fin) return FAILSOFT;
  }
  else fin = stdin;

  if (argc > 0 && *argv) {
    usage("too many arguments");
    r = FAILHARD;
    goto done;
  }

  if (setjmp(bufjump)) {
    printerr("input too big to sort");
    r = FAILSOFT;
    goto done;
  }

  if (verbosity > 0)
    fprintf(stderr, "(sorting with options: dict=%d, "
      "fold=%d, numeric=%d, reverse=%d, chunksize=%zd)\n",
      dictsort, casefold, numeric, reverse, lines.chunksize);

  if (lines.chunksize > 0) {
    r = extsort(&lines, fin, stdout);
  }
  else {
    r = memsort(&lines, fin, stdout);
  }

done:
  if (fin != stdin)
    fclose(fin);
  freelines(&lines);

  return r;
}

static int
memsort(struct lines *plines, FILE *fin, FILE *fout)
{
  readlines(plines, fin);
  CHECKIOERR(stdin, "reading input");

  sortlines(plines);

  writelines(plines, fout);
  CHECKIOERR(stdout, "writing output");

  return SUCCESS;
}

static int
extsort(struct lines *plines, FILE *fin, FILE *fout)
{
  int lo = 0, hi = -1, r;
  FILE *outfile, *infile;
  FILE *infiles[MERGEORDER];

  do {
    r = readlines(plines, fin);
    CHECKIOERR(stdin, "reading input");
    sortlines(plines);
    FILE *outfile = maketemp(++hi);
    if (!outfile) return FAILSOFT;
    writelines(plines, outfile);
    CHECKIOERR(outfile, "writing output");
    fclose(outfile);
  } while (r > 0);

  while (lo < hi) {
    int lim = MIN(lo+MERGEORDER-1, hi);
    opentemps(infiles, lo, lim);
    outfile = maketemp(++hi);
    if (!outfile) return FAILSOFT;
    merge(infiles, lim-lo+1, outfile);
    fclose(outfile);
    CHECKTMPERR(infiles, lim-lo+1, "merging");
    droptemps(infiles, lo, lim);
    lo += MERGEORDER;
  }

  infile = opentemp(hi);
  filecopy(infile, fout);
  fclose(infile);
  droptemp(hi);

  if (verbosity > 0)
    fprintf(stderr, "(external sorting used %d runs, "
    "chunk size = %zd, merge order = %d)\n", hi+1, plines->chunksize, MERGEORDER);

  return SUCCESS;
}

/* dictionary sort: any non-alnum is a separator */
#define ISSEP(c) (c && !isalnum(c))
#define SKIPSEP(s) while (ISSEP(*s)) ++s

static int
compare(const char *s, const char *t)
{
  int r, rev = reverse ? -1 : 1;

  if (numeric) {
    // compare numeric prefix; ignore leading space
    // lines w/o numeric prefix always sort at the end
    int i, j;
    s += scanspace(s);
    t += scanspace(t);
    size_t m = scanint(s, &i);
    size_t n = scanint(t, &j);
    if (m > 0 && n > 0) {
      if (i < j) return -1*rev;
      if (j < i) return +1*rev;
      s += m; t += n;
    }
    else if (m > 0) return -1;
    else if (n > 0) return +1;
    // numeric prefix equal (or both missing)
  }

  if (dictsort) {
    const unsigned char *ss = (void *) s;
    const unsigned char *tt = (void *) t;
    SKIPSEP(ss); SKIPSEP(tt);
    while (*ss && *tt) {
      if (*ss == *tt) { ++ss; ++tt; continue; }
      if (casefold && tolower(*ss) == tolower(*tt)) { ++ss; ++tt; continue; }
      int match = ISSEP(*ss) && ISSEP(*tt);
      SKIPSEP(ss); SKIPSEP(tt);
      if (!match) break;
    }
    SKIPSEP(ss); SKIPSEP(tt);
    r = casefold ? tolower(*ss) - tolower(*tt) : *ss - *tt;
  }
  else if (casefold) {
    const unsigned char *ss = (void *) s;
    const unsigned char *tt = (void *) t;
    while (*ss && *tt && (*ss == *tt || tolower(*ss) == tolower(*tt))) ++ss, ++tt;
    r = tolower(*ss) - tolower(*tt);
  }
  else r = strcmp(s, t);

  return r*rev;
}

static size_t /* one line from fp, NUL terminate, return #chars (w/o NUL) */
appendline(char **buf, FILE *fp)
{
  int c;
  size_t n0, n1;
  const int delim = '\n';

  n0 = buf_size(*buf);
  while ((c = getc(fp)) != EOF && c != delim) {
    buf_push(*buf, c);
  }
  if (c == delim) buf_push(*buf, delim);
  n1 = buf_size(*buf);

  /* fix incomplete last line */
  if (c == EOF && n1 > n0 && buf_peek(*buf) != delim) {
    buf_push(*buf, delim);
    n1 += 1;
  }

  buf_push(*buf, 0); /* terminate string */

  return ferror(fp) ? 0 : n1 - n0;
}

static int /* return <0 on error, 0 on eof */
readlines(struct lines *plines, FILE *fp)
{
  size_t pos = 0;
  size_t limit = plines->chunksize;

  buf_clear(plines->linebuf);
  buf_clear(plines->linepos);

  for (;;) {
    size_t n = appendline(&plines->linebuf, fp);
    if (n == 0) return 0; /* error or eof */
    buf_push(plines->linepos, pos);
    pos += n; /* advance position in linebuf */
    pos += 1; /* NUL is not counted by n */
    if (0 < limit && limit <= pos) return 1;
  }
}

static void /* sort the linepos array */
sortlines(struct lines *plines)
{
  size_t nlines = buf_size(plines->linepos);
  if (nlines > 0) {
    size_t lo = 0, hi = nlines-1;
    quick(plines->linepos, lo, hi, plines->linebuf);
  }
}

static void /* write lines in linepos-order to fp */
writelines(struct lines *plines, FILE *fp)
{
  size_t i, k, n;
  const char *s;
  n = buf_size(plines->linepos);
  for (i = 0; i < n; i++) {
    k = plines->linepos[i];
    s = plines->linebuf + k;
    fputs(s, fp);
  }
}

static void /* free the linebuf and linepos memory */
freelines(struct lines *plines)
{
  buf_free(plines->linebuf);
  buf_free(plines->linepos);
}

/* Temp file housekeeping */

/* generate name of temp file 'num' in given buffer */
static void nametemp(char *buf, size_t len, int num)
{
  const char *p;
  p = getenv("TMPDIR");
  if (!p) p = "/tmp";
  size_t n = snprintf(buf, len, "%s/sort%04d.tmp", p, num);
  assert(n < len); /* too long for given buffer */
}

/* assume temp fil e'num' does not exist and create it */
static FILE *maketemp(int num)
{
  char buf[PATHBUFLEN];
  nametemp(buf, sizeof buf, num);
  FILE *fp = fopen(buf, "w");
  if (!fp) printerr(buf);
  return fp;
}

/* assume temp file 'num' exists and open it for reading */
static FILE *opentemp(int num)
{
  char buf[PATHBUFLEN];
  nametemp(buf, sizeof buf, num);
  FILE *fp = fopen(buf, "r");
  if (!fp) printerr(buf);
  return fp;
}

/* assume temp file 'num' exists and delete it */
static void droptemp(int num)
{
  char buf[PATHBUFLEN];
  nametemp(buf, sizeof buf, num);
  int r = remove(buf);
  if (r < 0) printerr(buf);
}

/* assume temp files lo..hi exist and open them for reading */
static void opentemps(FILE **fps, int lo, int hi)
{
  int num;
  for (num = lo; num <= hi; num++) {
    fps[num-lo] = opentemp(num);
  }
}

/* close and delete temp flies lo..hi */
static void droptemps(FILE **fps, int lo, int hi)
{
  int num;
  for (num = lo; num <= hi; num++) {
    fclose(fps[num-lo]);
    droptemp(num);
  }
}

/* Merging */

struct run {
  FILE *fp;
  char *lp;
};

static int mergecmp(int i, int j, void *userdata)
{
  struct run *mergebuf = userdata;
  const char *s = mergebuf[i].lp;
  const char *t = mergebuf[j].lp;
  return compare(s, t);
}

static void merge(FILE *infps[], int numfp, FILE *outfp)
{
  struct run mergebuf[MERGEORDER];
  int heap[1+MERGEORDER]; /* indices into mergebuf; heap[0] unused */
  int n = 0; /* number of heap entries */

  for (int i = 0; i < numfp; i++) {
    FILE *fp = infps[i];
    char *lp = 0;
    size_t len = appendline(&lp, fp);
    if (len > 0) {
      mergebuf[n].fp = fp;
      mergebuf[n].lp = lp;
      heap[1+n] = n;
      n += 1;
    }
  }

  /* a fully sorted array is also a heap */
  quicksort(&heap[1], n, mergecmp, mergebuf);

  while (n > 0) {
    FILE *fp = mergebuf[heap[1]].fp;
    char *lp = mergebuf[heap[1]].lp;
    fputs(lp, outfp);
    buf_clear(lp);
    size_t len = appendline(&lp, fp);
    if (len > 0) {
      mergebuf[heap[1]].lp = lp;
    }
    else { /* one less input file */
      buf_free(lp);
      heap[1] = heap[n];
      n -= 1;
    }
    reheap(heap, n, mergecmp, mergebuf);
  }
}

/* Sorting algorithm */

static void swap(size_t *v, size_t i, size_t j);
static int linecmp(const size_t *v, size_t i, size_t j, const char *linebuf);

static void
quick(size_t *v, size_t lo, size_t hi, const char *linebuf)
{
  size_t i, lim;

  if (lo >= hi) return;    /* nothing to sort */

  swap(v, lo, (lo+hi)/2);  /* move middle elem as pivot to v[lo] */
  lim = lo;                /* invariant: v[lo..lim-1] < pivot */
  for (i = lo+1; i <= hi; i++)
    if (linecmp(v, i, lo, linebuf) < 0) /* if v[i] < pivot... */
      swap(v, ++lim, i);   /* ...swap it into left subset */
  swap(v, lo, lim);        /* restore pivot, NB v[lim] <= v[lo] */

  if (lim-lo < hi-lim) {   /* recurse smaller subset first */
    if (lim > 0) quick(v, lo, lim-1, linebuf);
    quick(v, lim+1, hi, linebuf);
  } else {
    quick(v, lim+1, hi, linebuf);
    if (lim > 0) quick(v, lo, lim-1, linebuf);
  }
  /* NB size_t is unsigned: watch for lim-1 wrapping around! */
}

static void
swap(size_t *v, size_t i, size_t j)
{
  size_t t = v[i];
  v[i] = v[j];
  v[j] = t;
}

static int
linecmp(const size_t *v, size_t i, size_t j, const char *linebuf)
{
  const char *s = linebuf + v[i];
  const char *t = linebuf + v[j];
  return compare(s, t);
}

/* Options and usage */

static int
parseopts(int argc, char **argv, size_t *chunksize)
{
  long l;
  int i, showhelp = 0;

  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'd': dictsort = true; break;
        case 'f': casefold = true; break;
        case 'n': numeric = true; break;
        case 'r': reverse = true; break;
        case 'c': 
          if (argv[i+1] && (l = atol(argv[i+1])) > 0 && !*(p+1)) {
            *chunksize = l;
            i += 1;
            break;
          }
          usage("option -c requires a positive number argument");
          return -1;
        case 'h': showhelp = 1;
          break;
        default: usage("invalid option");
          return -1;
      }
    }
  }

  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }

  return i; /* #args parsed */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-d] [-f] [-n] [-r] [-c bytes]\n", me);
  fprintf(fp, "Sort text lines\n");
  fprintf(fp, "  -c bytes   chunk size (in-memory sort if not specified)\n");
  fprintf(fp, "  -d   dictionary sort: compare only on letters and digits\n");
  fprintf(fp, "  -f   fold lower case and upper case (i.e., ignore case)\n");
  fprintf(fp, "  -n   numeric sort: assume first token is a number\n");
  fprintf(fp, "  -r   reverse sort\n");
}
