
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "lines.h"

static void shuffle(size_t v[], size_t n);
static int parseopts(int argc, char **argv, long *seed);
static void usage(const char *errmsg);

int
shufflecmd(int argc, char **argv)
{
  int r;
  size_t n;
  long seed = -1;
  struct lines lines = { 0, 0, 0 }; // must zero-init for buf.h

  r = parseopts(argc, argv, &seed);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  srand(seed >= 0 ? (unsigned) seed : (unsigned) time(0));

  if (verbosity > 0 && seed >= 0) {
    fprintf(stderr, "(random seed = %ld, RAND_MAX = %ld)\n", seed, (long) RAND_MAX);
  }

  if (argc == 0 || !*argv) {
    readlines(&lines, stdin);
  }
  else while (*argv) {
    const char *fn = *argv++;
    FILE *fp = openin(fn);
    if (!fp) goto ioerr;
    readlines(&lines, fp);
    fclose(fp);
  }

  n = countlines(&lines);
  shuffle(lines.linepos, n);

  writelines(&lines, stdout);

  freelines(&lines);
  return SUCCESS;

ioerr:
  freelines(&lines);
  return FAILSOFT;
}

static size_t random(size_t n)
{
  return rand() % n;
}

static void swap(size_t v[], size_t i, size_t j)
{
  size_t t = v[i]; v[i] = v[j]; v[j] = t;
}

/* random permutation (Fisher-Yates) */
static void shuffle(size_t v[], size_t n)
{
  while (n > 1) {
    size_t k = random(n); /* 0 <= k < n */
    n -= 1;
    swap(v, k, n);
  }
}

/* Options and usage */

static int
parseopts(int argc, char **argv, long *seed)
{
  long l;
  int i, showhelp = 0;

  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 's': 
          if (argv[i+1] && (l = atol(argv[i+1])) >= 0 && !*(p+1)) {
            *seed = l;
            i += 1;
            break;
          }
          usage("option -s requires a non-negative number argument");
          return -1;
        case 'h': showhelp = 1;
          break;
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

static void usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [-s seed]\n", me);
  fprintf(fp, "Shuffle text lines (random permutation)\n");
  fprintf(fp, "  -s seed   seed value for random number generator\n");
}
