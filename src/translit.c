
#include "common.h"

static int parseargs(int argc, char **argv,
  bool *allbut, const char **src, const char **dst);
static int xindex(bool allbut, const char *s, int c, int lastdst);
static int index(const char *s, int c);
static void usage(const char *msg);

int
translitcmd(int argc, char **argv)
{
  int c, i, r;
  bool allbut = false;
  const char *src = "";
  const char *dst = "";

  SHIFTARGS(argc, argv, 1);
  r = parseargs(argc, argv, &allbut, &src, &dst);
  if (r < 0) return FAILHARD;

  int dstlen = strlen(dst);
  int lastdst = dstlen - 1;
  int srclen = strlen(src);
  bool drop = dstlen <= 0; // dest is absent: drop src matches
  bool squash = srclen > dstlen; // src is longer: squash runs

  if (drop) {
    while ((c = getch()) != EOF) {
      i = xindex(allbut, src, c, lastdst);
      if (i < 0) putch(c); // copy; else: drop
    }
  }
  else do {
    i = xindex(allbut, src, c = getch(), lastdst);
    if (squash && i >= lastdst) { // squash
      putch(dst[lastdst]); // translate first
      do i = xindex(allbut, src, c = getch(), lastdst);
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
parseargs(int argc, char **argv,
  bool *allbut, const char **src, const char **dst)
{
  int i;
  for (i = 0; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; // no more option args
    if (streq(p, "--")) { ++i; break; } // end of option args
    for (++p; *p; p++) {
      switch (*p) {
        case 'c': *allbut = true; break;
        case 'h': usage(0); break;
        default: usage("invalid option"); return -1;
      }
    }
  }

  /* the src argument is mandatory */
  if (i < argc && argv[i]) {
    *src = argv[i++];
  }
  else {
    usage("the src argument is missing");
    return -1;
  }

  /* the dst argument is optional */
  if (i < argc && argv[i]) {
    *dst = argv[i++];
  }

  /* expect no more arguments */
  if (i < argc && argv[i]) {
    usage("too many arguments");
    return -1;
  }

  return argc;
}

/* xindex: conditionally invert result from index */
static int
xindex(bool allbut, const char *s, int c, int lastdst)
{
  if (c == EOF) return -1;
  if (!allbut) return index(s, c);
  if (index(s, c) >= 0) return -1;
  return lastdst+1; // request squashing
}

/* index: return position of c in s, -1 if not found */
static int
index(const char *s, int c)
{
  int i = 0;
  while (s[i] && s[i] != c) ++i;
  return s[i] == c ? i : -1;
}

/* usage: print usage and exit */
static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-c] src [dest]\n", me);
  fprintf(fp, "Transliterate, squash, or delete characters\n");
  exit(errmsg ? FAILHARD : SUCCESS);
}
