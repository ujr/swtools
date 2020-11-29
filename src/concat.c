
#include "common.h"

static int parseopts(int argc, char **argv);

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
  UNUSED(argc);
  UNUSED(argv);
  /* no options */
  return 1;
}
