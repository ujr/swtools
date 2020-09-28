
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "regex.h"
#include "strbuf.h"

static int getccl(const char *s, int i, strbuf *pat);
static int omatch(const char *lin, int i, const char *pat, int j);
static bool locate(char c, const char *pat, int j);
static int patsize(const char *pat, int i);

size_t
makepat(const char *s, int delim, strbuf *pat)
{
  size_t i = 0;
  while (s[i] != delim && s[i]) {
    char c = s[i];
    if (c == ANY) {
      strbuf_addc(pat, ANY);
      i++;
    }
    else if (c == BOL && i == 0) {
      strbuf_addc(pat, BOL);
      i++;
    }
    else if (c == EOL && s[i+1] == delim) {
      strbuf_addc(pat, EOL);
      i++;
    }
    else if (c == CCL) {
      i = getccl(s, i, pat);
    }
    else if (c == CLOSURE && i > 0) {
      printerr("regex: closure not yet implemented");
      abort();
    }
    else {
      strbuf_addc(pat, LITCHAR);
      strbuf_addc(pat, escape(s, &i));
    }
  }
  return i;
}

static int
getccl(const char *s, int i, strbuf *pat)
{
  size_t j, k, kk;
  i += 1; // skip over '['
  if (s[i] == NEGATE) {
    strbuf_addc(pat, NCCL);
    i += 1;
  }
  else {
    strbuf_addc(pat, CCL);
  }
  strbuf_addc(pat, '#'); // room for count
  k = strbuf_len(pat);
  j = dodash(s, i, CCLEND, pat);
  kk = strbuf_len(pat);
  strbuf_ptr(pat)[k-1] = kk - k;
  i = j+1; // skip over ']'
  return i;
}

/* match line against pat; return pos of match or -1 */
int
match(const char *line, const char *pat)
{
  int i, pos;
  if (!line || !pat) return -1;
  for (i=0, pos=-1; line[i] && pos < 0; i++) {
    pos = amatch(line, i, pat, 0);
  }
  return pos < 0 ? -1 : i-1;
}

/* anchored match: line[i..] against pat[j..] */
int
amatch(const char *line, int i, const char *pat, int j)
{
  /* PRELIMINARY - with some metacharacters */
  while (pat[j]) {
    int r = omatch(line, i, pat, j);
    if (r < 0) return -1; /* mismatch */
    i += r;
    j += patsize(pat, j);
  }
  return i; /* matched; next pos in input */
}

/* match one element; return 0 or 1 on match, -1 on mismatch */
static int
omatch(const char *lin, int i, const char *pat, int j)
{
  if (!lin[i]) return false;
  switch (pat[j]) {
    case LITCHAR:
      return lin[i] == pat[j+1] ? 1 : -1;
    case ANY:
      return lin[i] != '\n' && lin[i] != '\0' ? 1 : -1;
    case BOL:
      return i == 0 ? 0 : -1;
    case EOL:
      return lin[i] == '\n' || lin[i] == '\0' ? 0 : -1;
    case CCL:
      return locate(lin[i], pat, j+1) ? 1 : -1;
    case NCCL:
      return lin[i] != '\n' && lin[i] != '\0' && !locate(lin[i], pat, j+1) ? 1 : -1;
  }
  printerr("patsize; can't happen");
  abort();
}

/* look for c in character class at pat[j..] */
static bool
locate(char c, const char *pat, int j)
{
  // pat for ccl is: N c1 c2 c3 ... cN
  int i;
  for (i = j + pat[j]; i > j; i--) {
    if (c == pat[i])
      return true;
  }
  return false;
}

static int
patsize(const char *pat, int i)
{
  switch (pat[i]) {
    case LITCHAR: return 2;
    case BOL: return 1;
    case EOL: return 1;
    case ANY: return 1;
    case CCL: case NCCL:
      return 2 + pat[i+1];
    case CLOSURE:
      return CLOSIZE;
  }
  printerr("patsize: can't happen");
  abort();
}
