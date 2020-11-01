
#include <assert.h>
#include <ctype.h>

#include "common.h"

/* getprog: get program name from argument vector */
const char *
getprog(char **argv)
{
  const char *s = 0;
  register const char *p;
  if (argv && *argv) {
    p = s = *argv;
    while (*p) {
      if (*p++ == '/') {
        if (p) s = p; // advance
      }
    }
  }
  return (char *) s;
}

/* scanint: scan a decimal integer number, return #chars read */
size_t /* TODO consider int scanint(const char *s, int *pidx) */
scanint(const char *s, int *v)
{
  int neg, n;
  const char *p;

  if (!s) return 0;

  p = s;
  neg = 0;

  switch (*p) {
    case '-': neg=1; /* FALLTHRU */
    case '+': p+=1; break;
  }

  /* compute -n to get INT_MIN without overflow, but
     input outside INT_MIN..INT_MAX silently overflows */
  for (n = 0; isdigit(*p); p++) {
    n = 10 * n - (*p - '0');
  }

  if (v) *v = neg ? n : -n;
  return p - s; // #chars scanned
}

size_t
scanspace(const char *s)
{
  const char *p = s;
  while (isspace(*p)) ++p;
  return p - s; // #chars scanned
}

void
skipblank(const char *s, int *pidx)
{
  const char blank = ' ';
  const char tab = '\t';
  while (s[*pidx] == blank || s[*pidx] == tab) *pidx += 1;
}

/* escape: return escaped character at s[i], update i */
char
escape(const char *s, int *pi)
{
  char c = s[*pi];
  *pi += 1;
  if (c != ESC) return c; /* not escaped */
  c = s[*pi];
  if (c == 0) return ESC; /* not special at end */
  *pi += 1;
  switch (c) {
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
  return c;
}

/* dodash: expand dashes and escapes in s[i..] until delim, return i just after */
int
dodash(const char *s, int i, char delim, strbuf *buf)
{
  int i0 = i;
  while (s[i] && s[i] != delim) {
    if (s[i] == ESC)
      strbuf_addc(buf, escape(s, &i)); // escape
    else if (s[i] != DASH)
      strbuf_addc(buf, s[i++]); // normal character
    else if (i == i0 || s[i+1] == delim || s[i+1] == 0)
      strbuf_addc(buf, DASH), ++i; // literal dash at start or end
    else if (s[i-1] <= s[i+1]) { // character range
      for (char c = s[i-1]+1; c <= s[i+1]; c++) {
        strbuf_addc(buf, c);
      }
      i += 2; // skip dash and upper bound
    }
    else strbuf_addc(buf, s[i++]);
  }
  assert(buf->buf[buf->len] == '\0'); // guaranteed by strbuf
  return i;
}

/* openin: open existing file for reading, default to stdin */
FILE *
openin(const char *filepath)
{
  if (!filepath || streq(filepath, "-"))
    return stdin;
  FILE *fp = fopen(filepath, "rb");
  if (!fp) printerr(filepath);
  return fp;
}

/* openout: open file for writing, truncate if exists, default to stdout */
FILE *
openout(const char *filepath)
{
  if (!filepath || streq(filepath, "-"))
    return stdout;
  FILE *fp = fopen(filepath, "wb");
  if (!fp) printerr(filepath);
  return fp;
}

/* filecopy: copy file ifp to file ofp */
void
filecopy(FILE *ifp, FILE *ofp)
{
  /* TODO try fread/fwrite with a buffer of BUFSIZ (stdio.h) */
  int c;
  while ((c = getc(ifp)) != EOF) {
    putc(c, ofp);
  }
}

/* getline: read chars up to (and including) the first delim,
   return num chars read, 0 on EOF, -1 on error */
int
getline(strbuf *sp, int delim, FILE *fp)
{
  int c;
  size_t len;
  strbuf_trunc(sp, 0);
  while ((c = getc(fp)) != EOF && c != delim) {
    strbuf_addc(sp, c);
  }
  if (c == delim) strbuf_addc(sp, c);
  len = strbuf_len(sp);
  return strbuf_failed(sp) || ferror(fp) ? -1 : (int) len;
}

/* getln: read line from fp into buf[0..len-1], realloc if needed,
   return num chars read, 0 on EOF, -1 on error; caller must free(buf) */
int
getln(char **buf, size_t *len, FILE *fp)
{
  assert(buf != NULL);
  assert(len != NULL);
  assert(fp != NULL);

  char *p = *buf;
  size_t n = 0;
  size_t max = p ? *len : 0;
  int c;

  for (;;) {
    if (n+1 >= max) {
      max = 3*((max+16)/2);
      p = realloc(p, max);
      if (!p) return -1;
      *buf = p;
      *len = max;
    }
    c = getc(fp);
    if (c == '\n' || c == EOF) {
      if (c == '\n') p[n++] = c;
      p[n] = '\0';
      break;
    }
    p[n++] = c;
  }

  if (ferror(fp)) return -1;
  return (c == EOF && n == 0) ? 0 : n;
}

/* printerr: print system error message to stderr
   Syntax: "[me][: msg][: errno]\n" */
void
printerr(const char *msg)
{
  FILE *fp = stderr;
  extern const char *me;

  if (me) {
    fputs(me, fp);
  }

  if (msg && *msg) {
    fputs(": ", fp);
    fputs(msg, fp);
  }

  if (errno || !msg || !*msg) {
    fputs(": ", fp);
    fputs(strerror(errno), fp);
  }

  fputc('\n', fp);
}

/* checkioerr: check stream error flag, print error message */
int
checkioerr()
{
  if (ferror(stdin) || ferror(stdout)) {
    printerr(0);
    return FAILSOFT;
  }
  return SUCCESS;
}

