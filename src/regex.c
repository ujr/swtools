
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "regex.h"
#include "strbuf.h"

/* This regex code implements only a subset of regular
 * expressions, and it uses backtracking for closures,
 * so is quite slow. For real-world usage, consider
 * compiling the pattern into an automaton that can
 * match closures in a single pass over the input.
 */

#define CLOSURE '*'
#define BOL     '^'
#define EOL     '$'
#define ANY     '?'
#define CCL     '['
#define CCLEND  ']'
#define NEGATE  '^'
#define NCCL    '!'
#define LITCHAR 'c'

#define CLOSIZE 1 /* size of closure entry */

static void stclose(strbuf *pat, size_t i);
static int getccl(const char *s, int i, strbuf *pat);
static int omatch(const char *lin, int i, const char *pat, int j);
static bool locate(char c, const char *pat, int j);
static int patsize(const char *pat, int i);

size_t
makepat(const char *s, int delim, strbuf *pat)
{
  size_t i = 0;
  size_t lastj = strbuf_len(pat);
  while (s[i] != delim && s[i]) {
    size_t j = strbuf_len(pat);
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
      j = lastj;
      if ((c=strbuf_ptr(pat)[j]) == BOL || c == EOL || c == CLOSURE)
        return 0; /* invalid pattern: ^*, $*, ** are not allowed */
      strbuf_addc(pat, CLOSURE);
      /* rearrange pat so * comes before the element it applies to */
      stclose(pat, j);
      i++;
    }
    else {
      strbuf_addc(pat, LITCHAR);
      strbuf_addc(pat, escape(s, &i));
    }
    lastj = j;
  }
  return i;
}

/* insert pat[n-1] at pat[i] */
static void
stclose(strbuf *pat, size_t i)
{
  char *s = strbuf_ptr(pat);
  size_t len = strbuf_len(pat);
  size_t j = len-1; /* last index */
  char c = s[j];
  for (; j > i; j--)
    s[j] = s[j-1];
  s[j] = c;
}

static int
getccl(const char *s, int i, strbuf *pat)
{
  size_t j, k, kk;
  i += 1; /* skip over '[' */
  if (s[i] == NEGATE) {
    strbuf_addc(pat, NCCL);
    i += 1;
  }
  else {
    strbuf_addc(pat, CCL);
  }
  strbuf_addc(pat, '#'); /* room for count */
  k = strbuf_len(pat);
  j = dodash(s, i, CCLEND, pat);
  kk = strbuf_len(pat);
  strbuf_ptr(pat)[k-1] = kk - k;
  i = j+1; /* skip over ']' */
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

#if 0
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
#endif

/* anchored match: line[i..] against pat[j..] */
int
amatch(const char *lin, int i, const char *pat, int j)
{
  /* for each elem in pat:
  //   if elem is *:
  //     let n = as many omatch'es as possible
  //     for i = n downto 0:
  //       if amatch(rest of lin, rest of pat):
  //         return success
  //     return failure
  //   if not omatch: return failure
  // return success
  */
  int base, k, r;
  while (pat[j]) {
    if (pat[j] == CLOSURE) {
      j += patsize(pat, j);
      base = i;
      while (lin[i]) {
        r = omatch(lin, i, pat, j);
        if (r < 0) break;
        i += r;
      }
      /* now lin[i] is where the closure failed */
      /* match rest of lin against rest of pat */
      /* on failure, try again one char earlier */
      while (i >= base) {
        k = amatch(lin, i, pat, j+patsize(pat, j));
        if (k >= 0) break; /* matched */
        i -= 1; /* retry with closure one shorter */
      }
      return k;
    }
    r = omatch(lin, i, pat, j);
    if (r < 0) return -1;
    j += patsize(pat, j);
    i += r;
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
  /* pat for ccl is: N c1 c2 c3 ... cN */
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
