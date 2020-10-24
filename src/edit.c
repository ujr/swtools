
#include "common.h"

static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

int
editcmd(int argc, char **argv)
{
  const char *fn;
  int r;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  fn = 0;
  if (argc > 0 && *argv) {
    argc--;
    fn = *argv++;
  }

  if (argc > 0 && *argv) {
    usage("too many arguments");
    r = FAILHARD;
    goto done;
  }

  printf("This is edit, awaiting to be created (fn=%s)\n", fn);
  r = SUCCESS;

done:
  return r;
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
  fprintf(fp, "Usage: %s [file]\n", me);
  fprintf(fp, "Edit text files\n");
  fprintf(fp, "  -h   show this help text\n");
}
