
#include <ctype.h>

#include "common.h"
#include "strbuf.h"

static void include(FILE *fp, int *errcnt);
static bool parseline(const char *s, strbuf *sp);
static void usage(const char *errmsg);

int
includecmd(int argc, char **argv)
{
  const char *fn;
  FILE *fp;
  int errcnt = 0;

  SHIFTARGS(argc, argv, 1);

  fn = (argc > 0 && *argv) ? argc--, *argv++ : "-";
  if (argc > 0 || *argv) usage("expect at most one file");
  fp = openin(fn);
  if (!fp) return FAILSOFT;

  include(fp, &errcnt);

  if (ferror(stdout)) {
    printerr(0);
    errcnt++;
  }
  
  return errcnt > 0 ? FAILSOFT : SUCCESS;
}

static void
include(FILE *fp, int *errcnt)
{
  char *line = NULL;
  size_t len = 0;
  int n;
  strbuf filename = {0};

  while ((n = getln(&line, &len, fp)) > 0) {
    if (parseline(line, &filename)) {
      const char *fn2 = strbuf_buffer(&filename);
      FILE *fp2 = openin(fn2);
      if (fp2) {
        include(fp2, errcnt);
        fclose(fp2);
      }
      else {
        printf("#error %s: %s\n", fn2, strerror(errno));
        *errcnt+=1;
      }
    }
    else {
      putstr(line);
    }
  }

  if (n < 0 || strbuf_failed(&filename)) {
    printerr(0);
    *errcnt+=1;
  }

  if (line) free(line);
  strbuf_free(&filename);
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

/* usage: print usage and exit */
static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [file]\n", me);
  fprintf(fp, "Include files while copying input to standard output\n");
  exit(errmsg ? FAILHARD : SUCCESS);
}
