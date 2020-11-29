
#include "common.h"

static void dofile(FILE *fp, const char *fn, long *, long *, long *);
static void printcounts(long nl, long nw, long nc, const char *s);
static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

static const char *which = "lwc"; /* which counts to print */

int
countcmd(int argc, char **argv)
{
  int i, r;
  long nl, nw, nc;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc < 1) {
    dofile(stdin, 0, 0, 0, 0);
    r = checkioerr();
  }
  else {
    r = SUCCESS;
    nl = nw = nc = 0;
    for (i = 0; i < argc; i++) {
      const char *fn = argv[i];
      FILE *fp = openin(fn);
      if (fp) {
        dofile(fp, fn, &nl, &nw, &nc);
        if (ferror(fp)) {
          error("error reading %s", fn);
          r = FAILSOFT;
        }
        fclose(fp);
      }
      else{
        error("cannot open %s", fn);
        r = FAILSOFT;
      }
    }
    if (argc > 1)
      printcounts(nl, nw, nc, "total");
  }

  return r;
}

static void
dofile(FILE *fp, const char *fn, long *pnl, long *pnw, long *pnc)
{
  long nl, nw, nc;
  int c, inword;

  inword = 0;
  nl = nw = nc = 0;
  while ((c = fgetc(fp)) != EOF) {
    nc += 1;
    if (c > ' ' && c != 127) { /* ASCII */
      if (!inword) {
        nw += 1;
        inword = 1;
      }
    }
    else {
      inword = 0;
      if (c == '\n') nl += 1;
    }
  }

  if (pnl) *pnl += nl;
  if (pnw) *pnw += nw;
  if (pnc) *pnc += nc;

  printcounts(nl, nw, nc, fn);
}

static void
printcounts(long nl, long nw, long nc, const char *s)
{
  const char *fmt1 = "%8ld"; /* format for first count */
  const char *fmtn = " %7ld"; /* at least one blank between counts */

  const char *p = which;
  const char *fmt = fmt1;

  while (*p) {
    switch (*p++) {
      case 'l': printf(fmt, nl); break;
      case 'w': printf(fmt, nw); break;
      case 'c': printf(fmt, nc); break;
    }
    fmt = fmtn;
  }

  if (s) printf(" %s\n", s);
  else printf("\n");
}

static int /* return num args parsed */
parseopts(int argc, char **argv)
{
  const char *valid = "lwc";
  if (argc > 1 && argv[1] && *argv[1] == '-') {
    const char *p = argv[1]+1;
    if (p[0] == '-' && !p[1]) return 2; /* -- ends option args */
    if (p[0] == 'h' && !p[1]) { /* -h requests help */
      usage(0);
      exit(SUCCESS);
    }
    while (*p) {
      if (!strchr(valid, *p++)) {
        usage("invalid option");
        return -1; /* invalid invocation */
      }
    }
    which = argv[1]+1;
    return 2; /* the single option arg */
  }
  return 1; /* progname only */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-lwc] [file] ...\n", me);
  fprintf(fp, "Count lines, words, and bytes\n");
}
