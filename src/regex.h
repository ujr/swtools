#pragma once
#ifndef REGEX_H
#define REGEX_H

enum regex_flags {
  regex_none = 0,
  regex_ignorecase = 1
};

size_t makepat(const char *s, int delim, strbuf *pat);
int match(const char *line, const char *pat, int flags);
int amatch(const char *line, int i, const char *pat, int j, int flags);

#endif
