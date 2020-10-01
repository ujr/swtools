
#include <ctype.h>
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
#define ONEPLUS '+'
#define BOL     '^'
#define EOL     '$'
#define ANY     '.'
#define CCL     '['
#define CCLEND  ']'
#define NEGATE  '^'
#define NCCL    '!'
#define LITCHAR 'c'

#define CLOSIZE 1 /* size of closure entry (* and +) */

static void stclose(strbuf *pat, size_t i);
static int getccl(const char *s, int i, strbuf *pat);
static int omatch(const char *lin, int i, const char *pat, int j, int flags);
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
    else if ((c == CLOSURE || c == ONEPLUS) && i > 0) {
      char prev = strbuf_ptr(pat)[lastj];
      if (prev == BOL || prev == EOL || prev == CLOSURE || prev == ONEPLUS)
        return 0; /* invalid pattern: ^*, $*, ** are not allowed */
      strbuf_addc(pat, c);
      /* rearrange pat so * comes before the element it applies to */
      stclose(pat, lastj);
      j = lastj;
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
match(const char *line, const char *pat, int flags)
{
  int i, pos;
  if (!line || !pat) return -1;
  for (i=0, pos=-1; line[i] && pos < 0; i++) {
    pos = amatch(line, i, pat, 0, flags);
  }
  return pos < 0 ? -1 : i-1;
}

/* anchored match: line[i..] against pat[j..] */
int
amatch(const char *line, int i, const char *pat, int j, int flags)
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
  int elem, start, k, r;
  while ((elem = pat[j])) {
    if (elem == CLOSURE || elem == ONEPLUS) {
      j += patsize(pat, j);
      start = i;
      while (line[i]) {
        r = omatch(line, i, pat, j, flags);
        if (r < 0) break;
        i += r;
      }
      if (elem == ONEPLUS) {
        /* must match at least once */
        if (i == start) return -1;
        start += 1;
      }
      /* now lin[i] is where the closure failed */
      /* match rest of lin against rest of pat */
      /* on failure, try again one char earlier */
      while (i >= start) {
        k = amatch(line, i, pat, j+patsize(pat, j), flags);
        if (k >= 0) break; /* matched */
        i -= 1; /* retry with closure one shorter */
      }
      return k;
    }
    r = omatch(line, i, pat, j, flags);
    if (r < 0) return -1;
    j += patsize(pat, j);
    i += r;
  }
  return i; /* matched; next pos in input */
}

/* match one element; return 0 or 1 on match, -1 on mismatch */
static int
omatch(const char *line, int i, const char *pat, int j, int flags)
{
  int ignorecase = flags & regex_ignorecase;
  if (!line[i]) return false;
  switch (pat[j]) {
    case LITCHAR:
      return line[i] == pat[j+1] ||
        (ignorecase && tolower(line[i]) == tolower(pat[j+1])) ? 1 : -1;
    case ANY:
      return line[i] != '\n' && line[i] != '\0' ? 1 : -1;
    case BOL:
      return i == 0 ? 0 : -1;
    case EOL:
      return line[i] == '\n' || line[i] == '\0' ? 0 : -1;
    case CCL:
      return locate(line[i], pat, j+1) ? 1 : -1;
    case NCCL:
      return line[i] != '\n' && line[i] != '\0' &&
        !locate(line[i], pat, j+1) ? 1 : -1;
  }
  printerr("omatch: can't happen");
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
    case LITCHAR:
      return 2;
    case BOL:
    case EOL:
    case ANY:
      return 1;
    case CCL:
    case NCCL:
      return 2 + pat[i+1];
    case CLOSURE:
    case ONEPLUS:
      return CLOSIZE;
  }
  printerr("patsize: can't happen");
  abort();
}
