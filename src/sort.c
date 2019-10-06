
#include <stdio.h>
#include <setjmp.h>

#include "common.h"

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
};

static int appendline(char **buf, FILE *fp);
static int readlines(struct lines *plines, FILE *fp);
static void sortlines(struct lines *plines);
static void writelines(struct lines *plines, FILE *fp);
static void freelines(struct lines *plines);
static void quicksort(size_t *v, size_t lo, size_t hi, const char *linebuf);

int
sortcmd(int argc, char **argv)
{
  int r;
  struct lines lines = { 0, 0 }; // must zero-init for buf.h

  UNUSED(argc);
  UNUSED(argv);

  if (setjmp(bufjump)) {
    printerr("input too big to sort");
    r = FAILSOFT;
    goto done;
  }

  r = readlines(&lines, stdin);
  if (r < 0) {
    printerr("reading input");
    r = FAILSOFT;
    goto done;
  }

  sortlines(&lines);
  writelines(&lines, stdout);
  r = SUCCESS;

  if (ferror(stdout)) {
    printerr("writing output lines");
    r = FAILSOFT;
    goto done;
  }

done:
  freelines(&lines);

  return r;
}

static int /* one line from fp, NUL terminate, return #chars (w/o NUL) */
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
  buf_push(*buf, 0); /* terminate string */

  return ferror(fp) ? -1 : (int)(n1-n0);
}

static int /* return <0 on error, 0 on eof */
readlines(struct lines *plines, FILE *fp)
{
  size_t pos = 0;

  for (;;) {
    int n = appendline(&plines->linebuf, fp);
    if (n <= 0) return n; /* error or eof */
    buf_push(plines->linepos, pos);
    pos += n; /* advance position in linebuf */
    pos += 1; /* NUL is not counted by n */
  }
}

static void /* sort the linepos array */
sortlines(struct lines *plines)
{
  size_t nlines = buf_size(plines->linepos);
  if (nlines > 0) {
    size_t lo = 0, hi = nlines-1;
    quicksort(plines->linepos, lo, hi, plines->linebuf);
  }
}

static void /* write lines in linepos-order to fp */
writelines(struct lines *plines, FILE *fp)
{
  size_t i, j;
  const char *s;
  for (i = 0; i < buf_size(plines->linepos); i++) {
    j = plines->linepos[i];
    s = plines->linebuf + j;
    fputs(s, fp);
  }
}

static void /* free the linebuf and linepos memory */
freelines(struct lines *plines)
{
  buf_free(plines->linebuf);
  buf_free(plines->linepos);
}

/* Sorting algorithm */

static void swap(size_t *v, size_t i, size_t j);
static int compare(const size_t *v, size_t i, size_t j, const char *linebuf);

static void
quicksort(size_t *v, size_t lo, size_t hi, const char *linebuf)
{
  size_t i, lim;

  if (lo >= hi) return;    /* nothing to sort */

  swap(v, lo, (lo+hi)/2);  /* move middle elem as pivot to v[lo] */
  lim = lo;                /* invariant: v[lo..lim-1] < pivot */
  for (i = lo+1; i <= hi; i++)
    if (compare(v, i, lo, linebuf) < 0) /* if v[i] < pivot... */
      swap(v, ++lim, i);   /* ...swap it into left subset */
  swap(v, lo, lim);        /* restore pivot, NB v[lim] <= v[lo] */

  if (lim-lo < hi-lim) {   /* recurse smaller subset first */
    if (lim > 0) quicksort(v, lo, lim-1, linebuf);
    quicksort(v, lim+1, hi, linebuf);
  } else {
    quicksort(v, lim+1, hi, linebuf);
    if (lim > 0) quicksort(v, lo, lim-1, linebuf);
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
compare(const size_t *v, size_t i, size_t j, const char *linebuf)
{
  const char *s = linebuf + v[i];
  const char *t = linebuf + v[j];
  return strcmp(s, t);
}
