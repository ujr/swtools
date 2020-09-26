
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "regex.h"
#include "strbuf.h"

static bool getpat(const char *arg, strbuf *pat);
static void dofile(FILE *fp, const char *pat);
static int parseopts(int argc, char **argv);
static int usage(const char *errmsg);

static bool ignorecase = false;
static bool showlineno = false;
static bool invert = false;
static bool showhelp = false;

int
findcmd(int argc, char **argv)
{
  int r;
  const char *pat;
  strbuf patbuf = {0};

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (showhelp)
    return usage(0);

  if (!(argc > 0 && *argv))
    return usage("missing pattern argument");
  if (!getpat(*argv, &patbuf))
    return usage("invalid pattern argument");

  r = SUCCESS;
  pat = strbuf_ptr(&patbuf);

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      FILE *fp = openin(argv[i]);
      if (!fp) { r = FAILSOFT; continue; }
      dofile(fp, pat);
      if (ferror(fp)) {
        printerr(argv[i]);
        r = FAILSOFT;
      }
      fclose(fp);
    }
  }
  else {
    dofile(stdin, pat);
    if (ferror(stdin)) {
      printerr("error reading input");
      r = FAILSOFT;
    }
  }

  strbuf_free(&patbuf);

  return r;
}

static bool
getpat(const char *arg, strbuf *pat)
{
  strbuf_addz(pat, arg);
  return true;
}

static void
dofile(FILE *fp, const char *pat)
{
  const char *line;
  strbuf linebuf = {0};
  long lineno = 0;
  int pos;

  while (getline(&linebuf, '\n', fp) > 0) {
    lineno += 1;
    line = strbuf_ptr(&linebuf);
    pos = match(line, pat);
    if (invert ^ (pos >= 0)) {
      if (showlineno) fprintf(stdout, "%ld:%d ", lineno, pos+1);
      fputs(line, stdout);
    }
    strbuf_trunc(&linebuf, 0);
  }

  strbuf_free(&linebuf);
}

static int
parseopts(int argc, char **argv)
{
  int i;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "p")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'i': ignorecase = true; break;
        case 'n': showlineno = true; break;
        case 'v': invert = true; break;
        case 'h': showhelp = 1; break;
        default: usage("invalid option");
          return -1;
      }
    }
  }
  return i; /* #args parsed */
}

static int
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-i] [-n] [-v] pattern [files]\n", me);
  fprintf(fp, "Find and print lines matching the pattern\n");
  fprintf(fp, " -i  ignore case\n");
  fprintf(fp, " -n  prefix line number in output\n");
  fprintf(fp, " -v  emit lines that do not match\n");
  return errmsg ? FAILHARD : SUCCESS;
}
