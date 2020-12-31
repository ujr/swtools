/* translit - transliterate, squash, or delete characters */

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "common.h"
#include "strbuf.h"

static int parseargs(int argc, char **argv,
  bool *allbut, strbuf *src, strbuf *dst);
static void usage(const char *msg);

int
translitcmd(int argc, char **argv)
{
  int c, i, r;
  bool allbut = false;
  strbuf src = {0};
  strbuf dst = {0};

  r = parseargs(argc, argv, &allbut, &src, &dst);
  if (r < 0) return FAILHARD;

  const char *s = strbuf_ptr(&src);
  size_t slen = strbuf_len(&src);
  const char *d = strbuf_ptr(&dst);
  size_t dlen = strbuf_len(&dst);
  bool drop = dlen <= 0; /* dest absent: drop src matches */
  bool squash = slen > dlen || allbut; /* src longer: squash runs */
  int lastdst = dlen - 1; /* if squashing: use this char */

  if (drop) {
    while ((c = getch()) != EOF) {
      i = xindex(allbut, s, slen, c, lastdst);
      if (i < 0) putch(c); /* copy; else: drop */
    }
  }
  else do {
    i = xindex(allbut, s, slen, c = getch(), lastdst);
    if (squash && i >= lastdst) {
      putch(d[lastdst]); /* translate first char */
      do i = xindex(allbut, s, slen, c = getch(), lastdst);
      while (i >= lastdst); /* and drop remaining */
    }
    if (c != EOF) {
      if (i >= 0) putch(d[i]); /* translate */
      else putch(c); /* no match: copy */
    }
  } while (c != EOF);

  return checkioerr();
}

static int
parseargs(int argc, char **argv,
  bool *allbut, strbuf *src, strbuf *dst)
{
  int i;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'c': *allbut = true; break;
        case 'h': usage(0); break;
        default: usage("invalid option");
          return -1;
      }
    }
  }

  /* the src argument is mandatory */
  if (i < argc && argv[i]) {
    dodash(argv[i++], 0, '\0', src);
  }
  else {
    usage("the src argument is missing");
    return -1;
  }

  /* the dst argument is optional */
  if (i < argc && argv[i]) {
    dodash(argv[i++], 0, '\0', dst);
  }

  /* expect no more arguments */
  if (i < argc && argv[i]) {
    usage("too many arguments");
    return -1;
  }

  debug("src: \"%s\"", src->buf);
  debug("dst: \"%s\"", dst->buf);

  return argc;
}

int /* conditionally invert result from index */
xindex(bool allbut, const char *s, size_t slen, int c, int lastdst)
{
  if (c == EOF) return -1;
  if (!allbut) return index(s, slen, c);
  if (index(s, slen, c) >= 0) return -1;
  return lastdst+1; /* request squashing */
}

int /* index of c in s[slen] or -1 */
index(const char *s, size_t slen, int c)
{
  const char *p;
  if (!s) return -1;
  p = memchr(s, c, slen);
  return p ? p - s : -1;
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-c] src [dest]\n", me);
  fprintf(fp, "Transliterate, squash, or delete characters\n");
}
