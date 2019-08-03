
#include "common.h"

static const char *which = "lwc"; /* which counts to print */

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-lwc]\n", me);
  fprintf(fp, "Count lines, words, and bytes\n");
  exit(errmsg ? FAILHARD : SUCCESS);
}

static int /* return num args parsed */
parseargs(int argc, char **argv)
{
  const char *valid = "lwc";
  if (argc > 1 && argv[1] && *argv[1] == '-') {
    const char *p = argv[1]+1;
    if (p[0] == '-' && !p[1]) return 2; // -- ends option args
    if (p[0] == 'h' && !p[1]) usage(0);
    while (*p) {
      if (!strchr(valid, *p++)) {
        usage("invalid option");
      }
    }
    which = argv[1]+1;
    return 2; // the single option arg
  }
  return 1; // progname only
}

static void
printcounts(long nl, long nw, long nc)
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
}

int
countcmd(int argc, char **argv)
{
  int r, c, inword;
  long nl, nw, nc;

  r = parseargs(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r); // skip options

  inword = 0;
  nl = nw = nc = 0;
  while ((c = getch()) != EOF) {
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

  printcounts(nl, nw, nc);
  printf("\n");

  return checkioerr();
}
