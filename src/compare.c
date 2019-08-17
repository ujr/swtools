
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "strbuf.h"

static int parseopts(int argc, char **argv, bool *quiet);
static void usage(const char *errfmt, ...);
static FILE *openin(const char *filepath);
static bool equal(const char *line1, const char *line2);
static void diffmsg(int lineno, const char *line1, const char *line2);
static int getline(strbuf *sp, int delim, FILE *fp);
static void dumpline(const char *line, FILE *fp);

int
comparecmd(int argc, char **argv)
{
  int r;
  const char *fn1, *fn2;
  FILE *fp1, *fp2;
  int n1, n2;
  strbuf line1 = {0};
  strbuf line2 = {0};
  int lineno = 0;
  int numdiff = 0;
  bool quiet = 0;

  r = parseopts(argc, argv, &quiet);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc < 1 || argc > 2) {
    usage("expect one or two files");
    return FAILHARD;
  }

  fn1 = *argv++;
  fn2 = argc > 1 ? *argv++ : "-";

  if (streq(fn1, "-") && streq(fn2, "-")) {
    return 0;  /* identical */
  }

  fp1 = openin(fn1);
  fp2 = openin(fn2);
  if (!fp1 || !fp2) return FAILSOFT;

  for (;;) {
    lineno += 1;
    strbuf_trunc(&line1, 0);
    n1 = getline(&line1, '\n', fp1);
    strbuf_trunc(&line2, 0);
    n2 = getline(&line2, '\n', fp2);
    if (n1 <= 0 || n2 <= 0) break;
    if (!equal(strbuf_buffer(&line1), strbuf_buffer(&line2))) {
      numdiff += 1;
      if (!quiet)
        diffmsg(lineno, strbuf_buffer(&line1), strbuf_buffer(&line2));
    }
  }

  if (ferror(fp1) || ferror(fp2)) {
    printerr(0);
    return FAILSOFT;
  }

  if (numdiff == 0 && feof(fp1) && feof(fp2)) {
    return 0;  /* files are identical */
  }

  if (feof(fp1) && !feof(fp2) && !quiet) {
    printf("end of file on %s\n", fn1);
  }
  if (!feof(fp1) && feof(fp2) && !quiet) {
    printf("end of file on %s\n", fn2);
  }

  return 1;  /* files differ */
}

static int
parseopts(int argc, char **argv, bool *quiet)
{
  int i, showhelp = 0;

  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; // no more option args
    if (streq(p, "--")) { ++i; break; } // end of option args
    for (++p; *p; p++) {
      switch (*p) {
        case 'q': *quiet = true; break;
        case 'h': showhelp = 1; break;
        default: usage("invalid option: '%c'", *p); return -1;
      }
    }
  }

  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }

  return i; // #args parsed
}

/* openin: open filepath for reading, default to stdin */
static FILE *
openin(const char *filepath)
{
  if (!filepath || streq(filepath, "-"))
    return stdin;
  FILE *fp = fopen(filepath, "r");
  if (!fp) printerr(filepath);
  return fp;
}

/* equal: compare line1 and line2, return 1 if equal */
static bool
equal(const char *line1, const char *line2)
{
  if (line1 && line2) return strcmp(line1, line2) == 0;
  if (line1 || line2) return false;  /* either line is null */
  return true;  /* both lines are null */
}

static void
diffmsg(int lineno, const char *line1, const char *line2)
{
  printf("Line %d:\n", lineno);
  dumpline(line1, stdout);
  dumpline(line2, stdout);
}

/* getline: append chars up to (and including) the first delim,
   return num chars appended, 0 on EOF, -1 on error */
static int
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

/* dumpline: show line, substitute non-printing chars */
static void
dumpline(const char *line, FILE *fp)
{
  register const char *p;
  for (p=line; *p; p++) {
    if (isprint(*p)) putc(*p, fp);
    else switch (*p) {
      case '\a': putc('\\', fp); putc('a', fp); break;
      case '\b': putc('\\', fp); putc('b', fp); break;
      case '\f': putc('\\', fp); putc('f', fp); break;
      case '\n': putc('\\', fp); putc('n', fp); break;
      case '\r': putc('\\', fp); putc('r', fp); break;
      case '\t': putc('\\', fp); putc('t', fp); break;
      case '\v': putc('\\', fp); putc('v', fp); break;
      case '\\': putc('\\', fp); putc('\\', fp); break;
      default: fprintf(fp, "\\x%02x", (int)(unsigned char)*p); break;
    }
  }
  putc('\n', fp);
}

static void
usage(const char *errfmt, ...)
{
  char errmsg[256];
  va_list ap;
  FILE *fp = errfmt ? stderr : stdout;
  if (errfmt) {
    va_start(ap, errfmt);
    vsnprintf(errmsg, sizeof(errmsg), errfmt, ap);
    fprintf(fp, "%s: %s\n", me, errmsg);
    va_end(ap);
  }
  fprintf(fp, "Usage: %s [-q] file1 [file2]\n", me);
  fprintf(fp, "Compare files line-by-line\n");
  fprintf(fp, "Options: -q  quiet (do not report differing lines)\n");
  exit(errfmt ? FAILHARD : SUCCESS);
}

