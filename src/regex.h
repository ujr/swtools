#pragma once
#ifndef REGEX_H
#define REGEX_H

#include "strbuf.h"

enum regex_flags {
  regex_none = 0,
  regex_ignorecase = 1,
  regex_subjustone = 2 /* subline: first only (default: global) */
};

/* make s into a pattern for match(); return index of delim, -1 on error */
int makepat(const char *s, int delim, strbuf *pat);

/* match pat against line; return pos of match or -1 */
int match(const char *line, const char *pat, int flags);

/* anchored match; return index just after match or -1 */
int amatch(const char *line, int i, const char *pat, int j, int flags);

/* make s into a sub for subline(); return index of delim, -1 on error */
int makesub(const char *s, int delim, strbuf *sub);

/* substitute pat matches on line with sub */
int subline(const char *line, const char *pat, int flags, const char *sub, strbuf *out);

#endif
