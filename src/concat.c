/* concat - concatenate files */

#include "common.h"

static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

int
concatcmd(int argc, char **argv)
{
  int r;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc < 1 || !*argv) {
    filecopy(stdin, stdout);
    if (ferror(stdin)) {
      error("error reading input");
      return FAILSOFT;
    }
  }
  else while (argc > 0 && *argv) {
    FILE *fp = openin(*argv);
    if (!fp) return FAILSOFT;
    filecopy(fp, stdout);
    fclose(fp);
    if (ferror(fp)) {
      error("error reading %s", *argv);
      return FAILSOFT;
    }
    SHIFTARGS(argc, argv, 1);
  }

  if (ferror(stdout)) {
    error("error writing output");
    return FAILSOFT;
  }

  return SUCCESS;
}

static int
parseopts(int argc, char **argv)
{
  int i, showhelp = 0;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': showhelp = 1; break;
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
  fprintf(fp, "Usage: %s [files]\n", me);
  fprintf(fp, "Concatenate files (or stdin) to stdout\n");
}
