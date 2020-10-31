#pragma once
#ifndef REGEX_H
#define REGEX_H

#include "strbuf.h"

enum regex_flags {
  regex_none = 0,
  regex_ignorecase = 1
};

/* make s into a pattern for match(); return index of delim, 0 on error */
size_t makepat(const char *s, int delim, strbuf *pat);
void clearpat(strbuf *pat);

/* match pat against line; return pos of match or -1 */
int match(const char *line, const char *pat, int flags);

/* anchored match; return index just after match or -1 */
int amatch(const char *line, int i, const char *pat, int j, int flags);

size_t makesub(const char *s, int delim, strbuf *sub);
void subline(const char *line, const char *pat, int flags, const char *sub, strbuf *out);

#endif
