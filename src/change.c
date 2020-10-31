
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "regex.h"
#include "strbuf.h"

static bool getpat(const char *arg, strbuf *pat);
static bool getsub(const char *arg, strbuf *sub);
static int parseopts(int argc, char **argv);
static int usage(const char *errmsg);

static bool showhelp = false;
static bool ignorecase = false;

int
changecmd(int argc, char **argv)
{
  int r;
  int flags = regex_none;
  strbuf patbuf = {0};
  strbuf subbuf = {0};
  strbuf linebuf = {0};
  strbuf outbuf = {0};
  const char *pat;
  const char *sub;
  const char *line;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (showhelp)
    return usage(0);

  if (!*argv)
    return usage("missing pattern argument");
  if (!getpat(*argv++, &patbuf))
    return usage("invalid pattern argument");

  if (*argv && !getsub(*argv++, &subbuf))
    return usage("invalid replacement argument");

  if (*argv)
    return usage("too many arguments");

  pat = strbuf_ptr(&patbuf);
  sub = strbuf_ptr(&subbuf);

  if (ignorecase)
    flags |= regex_ignorecase;

  while (getline(&linebuf, '\n', stdin) > 0) {
    line = strbuf_ptr(&linebuf);
    subline(line, pat, flags, sub, &outbuf);
    fputs(strbuf_ptr(&outbuf), stdout);
    strbuf_trunc(&outbuf, 0);
  }

  strbuf_free(&linebuf);
  strbuf_free(&patbuf);

  return checkioerr();
}

static bool
getpat(const char *arg, strbuf *pat)
{
  strbuf_trunc(pat, 0);
  return makepat(arg, '\0', pat) > 0;
}

static bool
getsub(const char *arg, strbuf *sub)
{
  strbuf_trunc(sub, 0);
  return makesub(arg, '\0', sub) > 0;
}

static int
parseopts(int argc, char **argv)
{
  int i;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'i': ignorecase = 1; break;
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
  fprintf(fp, "Usage: %s pattern [replacement]\n", me);
  fprintf(fp, "Change patterns in text; the matching input will\n");
  fprintf(fp, "be substituted for each & in the replacement text.\n");
  return errmsg ? FAILHARD : SUCCESS;
}
