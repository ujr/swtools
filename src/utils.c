
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
size_t
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

/* getline: append chars up to (and including) the first delim,
   return num chars appended, 0 on EOF, -1 on error */
int
getline(strbuf *sp, int delim, FILE *fp)
{
  int c;
  size_t l0, l1;
  l0 = strbuf_length(sp);
  while ((c = getc(fp)) != EOF && c != delim) {
    strbuf_addc(sp, c);
  }
  if (c == delim) strbuf_addc(sp, c);
  l1 = strbuf_length(sp);
  return strbuf_failed(sp) || ferror(fp) ? -1 : (int)(l1 - l0);
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

