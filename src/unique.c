
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "strbuf.h"

static int unique(FILE *fin, bool count, FILE *fout);
static bool equal(strbuf *sp1, strbuf *sp2);
static int parseargs(int argc, char **argv, bool *count);
static void usage(const char *msg);

int
uniquecmd(int argc, char **argv)
{
  int r;
  bool count = 0;
  const char *fn;
  FILE *fp;

  r = parseargs(argc, argv, &count);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc > 0 && *argv) {
    fn = *argv++, argc--;
    fp = openin(fn);
    if (!fp) return FAILSOFT;
  }
  else fp = stdin;

  if (argc > 0 && *argv) {
    usage("too many arguments");
    goto done;
  }

  r = unique(fp, count, stdout);

done:
  if (fp != stdin)
    fclose(fp);

  return r;
}

static int
unique(FILE *fin, bool count, FILE *fout)
{
  strbuf sb0 = {0}, *sp0 = &sb0;
  strbuf sb1 = {0}, *sp1 = &sb1;
  size_t n, num;
  int r;

  n = getline(sp0, '\n', fin);
  if (n <= 0) goto done;
  num = 1;

  while ((n = getline(sp1, '\n', fin)) > 0) {
    if (equal(sp0, sp1)) num += 1;
    else {
      strbuf *temp;
      if (count) fprintf(fout, "%zd\t", num);
      fputs(strbuf_ptr(sp0), fout);
      temp = sp0; sp0 = sp1; sp1 = temp;
      num = 1;
    }
  }

  if (count) fprintf(fout, "%zd\t", num);
  fputs(strbuf_ptr(sp0), fout);

done:
  strbuf_free(sp0);
  strbuf_free(sp1);

  r = SUCCESS;
  if (ferror(fin)) {
    error("error on input");
    r = FAILSOFT;
  }
  if (ferror(fout)) {
    error("error on output");
    r = FAILSOFT;
  }

  return r;
}

static bool
equal(strbuf *sp1, strbuf *sp2)
{
  const char *sz1, *sz2;
  assert(sp1 != NULL && sp2 != NULL);
  sz1 = strbuf_ptr(sp1);
  sz2 = strbuf_ptr(sp2);
  return strcmp(sz1, sz2) == 0;
}

static int
parseargs(int argc, char **argv, bool *count)
{
  int i, showhelp = 0;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'n': *count = 1; break;
        case 'h': showhelp = 1; break;
        default: usage("invalid option");
          return -1;
      }
    }
  }
  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }
  return i; /* #args parsed */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-n] [file]\n", me);
  fprintf(fp, "Delete adjacent duplicate lines\n");
  fprintf(fp, "  -n   prefix number of occurrences\n");
}
