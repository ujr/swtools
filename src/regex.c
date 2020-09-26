
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "regex.h"

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
  /* PRELIMINARY - with no metacharacters */
  while (pat[j]) {
    if (line[i] != pat[j]) return -1; /* no match */
    i++;
    j++;
  }
  return i; /* matched; next pos in input */
}
