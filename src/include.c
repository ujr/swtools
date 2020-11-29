
#include <ctype.h>

#include "common.h"
#include "strbuf.h"

static void include(FILE *fp, const char *fn, int level, int *errcnt);
static bool parseline(const char *s, strbuf *sp);

static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

static int maxdepth = 5;

int
includecmd(int argc, char **argv)
{
  const char *fn;
  FILE *fp;
  int r, errcnt = 0;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc <= 0 || !*argv) { /* stdin */
    include(stdin, "-", 0, &errcnt);
  }
  else while (argc > 0 && *argv) { /* all args */
    fn = *argv++, argc--;
    fp = openin(fn);
    if (!fp) return FAILSOFT;
    include(fp, fn, 0, &errcnt);
  }

  if (ferror(stdout)) {
    error("error writing output");
    errcnt++;
  }

  return errcnt > 0 ? FAILSOFT : SUCCESS;
}

static void
include(FILE *fp, const char *fn, int level, int *errcnt)
{
  strbuf linebuf = {0};
  strbuf namebuf = {0};
  int n;

  if (level > maxdepth)
    fatal("max inclusion depth of %d exceeded", maxdepth);

  while ((n = getline(&linebuf, '\n', fp)) > 0) {
    const char *line = strbuf_ptr(&linebuf);
    if (parseline(line, &namebuf)) {
      const char *fn2 = strbuf_ptr(&namebuf);
      FILE *fp2 = openin(fn2);
      if (fp2) {
        include(fp2, fn2, level+1, errcnt);
        fclose(fp2);
      }
      else {
        message("#error %s: %s", fn2, strerror(errno));
        *errcnt+=1;
      }
    }
    else {
      putstr(line);
    }
  }

  if (ferror(fp)) {
    error("error reading %s", fn);
    errcnt += 1;
  }
  if (strbuf_failed(&namebuf)) {
    error("out of memory");
    *errcnt+=1;
  }

  strbuf_free(&linebuf);
  strbuf_free(&namebuf);
}

/* parseline: see if this is a #include line, get the file name */
static bool
parseline(const char *line, strbuf *outname)
{
  const char *p, *q;
  ptrdiff_t n;
  p = line;
  while (isspace(*p)) ++p;
  if (*p++ != '#') return false;
  while (isspace(*p)) ++p;
  if (strncmp(p, "include", 7)) return false;
  p += 7;
  while (isspace(*p)) ++p;
  if (*p++ != '"') return false;
  q = p;
  while (*p && *p != '"') ++p;
  n = p - q;
  if (*p != '"') return false;
  strbuf_trunc(outname, 0);
  strbuf_addb(outname, q, (size_t) n);
  return true;
}

static int
parseopts(int argc, char **argv)
{
  int i, showhelp = 0;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': showhelp = 1; break;
        default: usage("invalid option"); return -1;
      }
    }
  }
  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }
  return i; /* #args parsed */
}

/* usage: print usage and exit */
static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [file]\n", me);
  fprintf(fp, "Include files while copying input to standard output\n");
  fprintf(fp, "Specify include files as: #include \"filename\"\n");
  exit(errmsg ? FAILHARD : SUCCESS);
}
