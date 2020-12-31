/* print - dump arbitrary input to stdout */

#include <ctype.h>
#include <unistd.h> /* POSIX */

#include "common.h"

/*
Non printing characters:
  \a \b \f \n \r \t \v \XX (upper hex)

Offset   Line Dump
00000000 0001 ...\
0000001C 0001 ...$
*/

#define MAXLINE 72

static int parseopts(int argc, char **argv,
  bool *ansiterm, bool *showeol, bool *newline,
  bool *allhex, bool *lines, bool *offsets);
static void filedump(FILE *ifp, FILE *ofp);
static void metaputc(int c, FILE *fp);
static int metaputx(int c, FILE *fp);
static void metaprefix(unsigned long offset, unsigned long lineno, FILE *fp);
static void usage(const char *errmsg);

static bool ansiterm = false;
static bool showeol = false;
static bool newline = true;
static bool allhex = false;
static bool linenums = false;
static bool offsets = false;

static unsigned long offset = 0;
static unsigned long lineno = 1;
static unsigned long linepos = 0;
static unsigned int maxline = MAXLINE;

int
printcmd(int argc, char **argv)
{
  int r;

  ansiterm = isatty(STDOUT_FILENO);
  r = parseopts(argc, argv, &ansiterm, &showeol, &newline,
                &allhex, &linenums, &offsets);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  /* make room for offset/line prefix: */
  if (offsets) maxline -= 9;
  if (linenums) maxline -= 5;

  if (argc < 1 || !*argv) {
    filedump(stdin, stdout);
    if (ferror(stdin) || ferror(stdout)) {
      error("I/O error");
      return FAILSOFT;
    }
    return SUCCESS;
  }

  while (argc > 0 && *argv) {
    FILE *fp = openin(*argv);
    if (!fp) return FAILSOFT;
    filedump(fp, stdout);
    fclose(fp);
    if (ferror(fp)) {
      error("error reading %s", *argv);
      return FAILSOFT;
    }
    SHIFTARGS(argc, argv, 1);
  }

  if (ferror(stdout)) {
    error("error writing output");
    return FAILSOFT;
  }

  return SUCCESS;
}

static void
filedump(FILE *ifp, FILE *ofp)
{
  int c, n;
  metaprefix(offset, lineno, ofp);
  while ((c = getc(ifp)) != EOF) {
    offset += 1;
    if (c == '\n') {
      if (!newline) metaputx('\n', ofp);
      if (showeol) metaputc('$', ofp);
      putc('\n', ofp);
      lineno++;
      linepos = 0;
      metaprefix(offset, lineno, ofp);
    }
    else if (isprint(c)) {
      putc(c, ofp);
      linepos++;
    }
    else {
      n = metaputx(c, ofp);
      linepos += n;
    }

    if (linepos >= maxline) {
      if (showeol) metaputc('\\', ofp);
      putc('\n', ofp);
      linepos = 0;
      metaprefix(offset, lineno, ofp);
    }
  }

  if (linepos > 0 || offsets || linenums) putc('\n', ofp);
}

#define ANSIBLUE  "\033[34m"
#define ANSIBOLD  "\033[1m"
#define ANSIRESET "\033[0m"

static void
metaprefix(unsigned long offset, unsigned long lineno, FILE *fp)
{
  if (offsets) {
    if (ansiterm) fputs(ANSIBLUE, fp);
    fprintf(fp, "%08lx ", offset);
    if (ansiterm) fputs(ANSIRESET, fp);
  }

  if (linenums) {
    if (ansiterm) fputs(ANSIBLUE, fp);
    fprintf(fp, "%04ld ", lineno);
    if (ansiterm) fputs(ANSIRESET, fp);
  }
}

static void
metaputc(int c, FILE *fp)
{
  if (ansiterm)
    fputs(ANSIBLUE ANSIBOLD, fp);
  putc(c, fp);
  if (ansiterm)
    fputs(ANSIRESET, fp);
}

static int
metaputx(int c, FILE *fp)
{
  const char *p;
  int n;
  if (ansiterm) fputs(ANSIBLUE ANSIBOLD, fp);
  if (!allhex && (p = strchr("a\ab\bf\fn\nr\rt\tv\v0\0", c))) {
    putc('\\', fp);
    putc(*(p-1), fp);
    n = 2;
  }
  else n = fprintf(fp, "\\%02X", c);
  if (ansiterm) fputs(ANSIRESET, fp);
  return ferror(fp) ? 0 : n;
}

static int
parseopts(int argc, char **argv,
  bool *ansiterm, bool *showeol, bool *newline, bool *allhex,
  bool *linenums, bool *offsets)
{
  int i, showhelp = 0;

  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'e': *showeol = 1; break;
        case 'n': *newline = 0; break;
        case 'x': *allhex = 1; break;
        case 'l': *linenums = 1; break;
        case 'o': *offsets = 1; break;
        case 'a': *ansiterm = 1; break;
        case 'A': *ansiterm = 0; break;
        case 'h': showhelp = 1; break;
        default: usage("invalid option"); return -1;
      }
    }
  }

  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }

  return i;  /* #args parsed */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-aAelox] [file] ...\n", me);
  fprintf(fp, "Dump arbitrary files human readable to standard output\n");
  fprintf(fp, "Options: -a (ansi color on), -A (ansi color off),\n");
  fprintf(fp, "  -o (show offsets), -l (show line numbers),\n");
  fprintf(fp, "  -e (show eol), -n (escape newlines), -x (hex escapes only)\n");
}
