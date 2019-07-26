
#include "common.h"

static int parseargs(int argc, char **argv, const char **src, const char **dst);
static int index(const char *s, int c);
static void usage(const char *msg);

int
translitcmd(int argc, char **argv)
{
  int c, i, r;
  const char *src, *dst;

  SHIFTARGS(argc, argv, 1);
  r = parseargs(argc, argv, &src, &dst);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  int dstlen = strlen(dst);
  int lastdst = dstlen - 1;
  int srclen = strlen(src);
  bool drop = dstlen <= 0; // dest is absent: drop src matches
  bool squash = srclen > dstlen; // src is longer: squash runs

  if (drop) {
    while ((c = getch()) != EOF) {
      i = index(src, c);
      if (i < 0) putch(c); // copy; else: drop
    }
  }
  else do {
    i = index(src, c = getch());
    if (squash && i >= lastdst) { // squash
      putch(dst[lastdst]); // translate first
      do i = index(src, c = getch());
      while (i >= lastdst); // drop remaining
    }
    if (c != EOF) {
      if (i >= 0) putch(dst[i]); // translate
      else putch(c); // no match: copy
    }
  } while (c != EOF);

  return checkioerr();
}

static int
parseargs(int argc, char **argv, const char **src, const char **dest)
{
  if (argc < 1 || argc > 2) {
    usage("expect one or two arguments");
    return -1;
  }

  *src = argv[0];
  *dest = argc > 1 ? argv[1] : "";

  return argc;
}

/* index of c in s, -1 if not found */
static int
index(const char *s, int c)
{
  int i = 0;
  while (s[i] && s[i] != c) ++i;
  return s[i] == c ? i : -1;
}

/* print usage and exit */
static void
usage(const char *msg)
{
  FILE *fp = msg ? stderr : stdout;
  if (msg) fprintf(fp, "%s: %s\n", me, msg);
  fprintf(fp, "Usage: %s src [dest]\n", me);
  exit( msg ? FAILHARD : SUCCESS);
}
