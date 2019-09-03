
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
    if (ferror(stdin) || ferror(stdout)) {
      printerr(0);
      return FAILSOFT;
    }
    return SUCCESS;
  }

  while (argc > 0 && *argv) {
    FILE *fp = openin(*argv);
    if (!fp) return FAILSOFT;
    filecopy(fp, stdout);
    fclose(fp);
    if (ferror(fp)) {
      printerr(*argv);
      return FAILSOFT;
    }
    SHIFTARGS(argc, argv, 1);
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
