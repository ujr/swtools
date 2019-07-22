
#include "common.h"

static void output(const char *s);
static int parseopts(int argc, char **argv, int *escapes, int *newline);
static void usage(const char *msg);

int
echocmd(int argc, char **argv)
{
  const char blank = ' ';
  int i, r;
  int escapes = 0; // ignore escape sequences in arguments
  int newline = 1; // output trailing newline

  SHIFTARGS(argc, argv, 1);
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
    printerr(0);
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

  for (i = 0; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-') break; // no more option args
    for (++p; *p; p++) {
      switch (*p) {
        case 'e': *escapes = 1; break;
        case 'n': *newline = 0; break;
        case 'h': usage(0); break;
        case '-': return ++i; // end of option args
        default: usage("invalid option"); return -1;
      }
    }
  }

  return i; // #args parsed
}

static void
usage(const char *msg)
{
  FILE *fp = msg ? stderr : stdout;
  if (msg) fprintf(fp, "%s: %s\n", me, msg);
  fprintf(fp, "Usage: %s [-e] [-n] [arguments...]\n", me);
  exit(msg ? FAILHARD : SUCCESS);
}
