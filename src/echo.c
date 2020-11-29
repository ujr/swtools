
#include "common.h"

static void output(const char *s);
static int parseopts(int argc, char **argv, int *escapes, int *newline);
static void usage(const char *msg);

int
echocmd(int argc, char **argv)
{
  const char blank = ' ';
  int i, r;
  int escapes = 0; /* ignore escapes in arguments */
  int newline = 1; /* output trailing newline */

  r = parseopts(argc, argv, &escapes, &newline);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  for (i = 0; i < argc && argv[i]; i++) {
    if (i > 0) putch(blank);
    if (escapes) output(argv[i]);
    else putstr(argv[i]);
  }

  if (newline) putch('\n');

  if (ferror(stdout)) {
    error("error writing output");
    return FAILSOFT;
  }

  return SUCCESS;
}

static void
output(const char *s)
{
  static const char *esc = "\\\\a\ab\be\033f\fn\nr\rt\tv\v0\0";
  const char *p, *q;

  for (p = s; *p; p++) {
    if (*p == '\\' && p[1]) {
      q = strchr(esc, *++p);
      if (q) putch(q[1]);
      else putch(*p);
    }
    else putch(*p);
  }
}

static int
parseopts(int argc, char **argv, int *escapes, int *newline)
{
  int i;

  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'e': *escapes = 1; break;
        case 'n': *newline = 0; break;
        case 'h': usage(0); exit(SUCCESS); break;
        default: usage("invalid option"); return -1;
      }
    }
  }

  return i; /* #args parsed */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-e] [-n] [arguments...]\n", me);
  fprintf(fp, "Copy arguments to standard output\n");
  fprintf(fp, "Options: -n (omit trailing newline)\n");
  fprintf(fp, "  -e (enable \\\\ \\a \\b \\e \\f \\n \\r \\t \\v \\0)\n");
}
