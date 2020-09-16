
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>

#include "common.h"
#include "strbuf.h"

#define ESC '\\'
#define DASH '-'

static int parseargs(int argc, char **argv,
  bool *allbut, strbuf *src, strbuf *dst);
static int dodash(const char *s, char delim, strbuf *buf);
static char escape(const char *s, int *n);
static int xindex(bool allbut, strbuf *sb, int c, int lastdst);
static int index(const char *s, size_t slen, int c);
static void usage(const char *msg);

int
translitcmd(int argc, char **argv)
{
  int c, i, r;
  bool allbut = false;
  strbuf src = {0};
  strbuf dst = {0};

  /* default src and dest to empty */
  strbuf_addz(&src, "");
  strbuf_addz(&dst, "");

  r = parseargs(argc, argv, &allbut, &src, &dst);
  if (r < 0) return FAILHARD;
  if (strbuf_failed(&src) || strbuf_failed(&dst)) {
    printerr("expanding src/dest arguments");
    return FAILSOFT;
  }

  int dstlen = strbuf_len(&dst);
  int srclen = strbuf_len(&src);
  bool drop = dstlen <= 0; // dest is absent: drop src matches
  bool squash = srclen > dstlen; // src is longer: squash runs
  int lastdst = dstlen - 1; // if squashing: use this character

  if (drop) {
    while ((c = getch()) != EOF) {
      i = xindex(allbut, &src, c, lastdst);
      if (i < 0) putch(c); // copy; else: drop
    }
  }
  else do {
    i = xindex(allbut, &src, c = getch(), lastdst);
    if (squash && i >= lastdst) { // squash
      putch(dst.buf[lastdst]); // translate first
      do i = xindex(allbut, &src, c = getch(), lastdst);
      while (i >= lastdst); // drop remaining
    }
    if (c != EOF) {
      if (i >= 0) putch(dst.buf[i]); // translate
      else putch(c); // no match: copy
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
    dodash(argv[i++], '\0', src);
  }
  else {
    usage("the src argument is missing");
    return -1;
  }

  /* the dst argument is optional */
  if (i < argc && argv[i]) {
    dodash(argv[i++], '\0', dst);
  }

  /* expect no more arguments */
  if (i < argc && argv[i]) {
    usage("too many arguments");
    return -1;
  }

  if (verbosity > 0) {
    fprintf(stderr, "src: \"%s\"\n", src->buf);
    fprintf(stderr, "dst: \"%s\"\n", dst->buf);
  }

  return argc;
}

/* dodash: expand dashes and escapes in sz until delim */
static int
dodash(const char *sz, char delim, strbuf *buf)
{
  const char *p = sz; int n;
  while (*p && *p != delim) {
    if (*p == ESC) { strbuf_addc(buf, escape(p, &n)); p += n; } // escape
    else if (*p != DASH) strbuf_addc(buf, *p++); // normal character
    else if (p == sz || p[1] == 0) strbuf_addc(buf, *p++); // literal dash
    else if (p[-1] <= p[+1]) { // character range
      for (char c = p[-1]+1; c <= p[1]; c++) {
        strbuf_addc(buf, c);
      }
      p += 2; // skip dash and upper bound
    }
    else strbuf_addc(buf, *p++);
  }
  assert(buf->buf[buf->len] == '\0'); // this is guaranteed by strbuf
  return p-sz; // #chars scanned
}

/* escape: return escaped character at *s, set n to #chars scanned */
static char
escape(const char *sz, int *n)
{
  *n = 1;
  if (sz[0] != ESC) return *sz; // not escaped
  if (sz[1] == 0) return ESC; // not special at end
  *n += 1;
  switch (sz[1]) {
    case '\\': return '\\';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'e': return '\033';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '0': return '\0';
  }
  return sz[1];
}

/* xindex: conditionally invert result from index */
static int
xindex(bool allbut, strbuf *sb, int c, int lastdst)
{
  if (c == EOF) return -1;
  if (!allbut) return index(sb->buf, sb->len, c);
  if (index(sb->buf, sb->len, c) >= 0) return -1;
  return lastdst+1; // request squashing
}

/* index: return position of c in s, -1 if not found */
static int
index(const char *s, size_t slen, int c)
{
  size_t i = 0;
  if (!s) return -1;
  //while (s[i] && s[i] != c) ++i;
  while (i < slen && s[i] != c) ++i;
  return s[i] == c ? (int) i : -1;
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
