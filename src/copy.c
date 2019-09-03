
#include "common.h"

static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

int
copycmd(int argc, char **argv)
{
  FILE *ifp = stdin;
  FILE *ofp = stdout;
  int r;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc > 2) {
    usage("too many arguments");
    return FAILHARD;
  }

  if (argc > 0) {
    ifp = openin(argv[0]);
    if (!ifp) return FAILSOFT;
  }

  if (argc > 1) {
    ofp = openout(argv[1]);
    if (!ofp) return FAILSOFT;
  }

  filecopy(ifp, ofp);

  r = 0;

  if (fflush(ofp) == EOF || ferror(ofp)) {
    printerr("output error");
    r += 1;
  }

  if (ferror(ifp)) {
    printerr("input error");
    r += 1;
  }

  if (ofp != stdout) fclose(ofp);
  if (ifp != stdin) fclose(ifp);

  return r == 0 ? SUCCESS : FAILSOFT;
}

static int
parseopts(int argc, char **argv)
{
  int i;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; // no more option args
    if (streq(p, "--")) { ++i; break; } // end of option args
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': usage(0); break;
        default: usage("invalid option"); return -1;
      }
    }
  }

  return i; // #args parsed
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [infile [outfile]]\n", me);
  fprintf(fp, "Copy infile (or stdin) to outfile (or stdout)\n");
  fprintf(fp, "The outfile is first created or truncated\n");
  exit(errmsg ? FAILHARD : SUCCESS);
}
