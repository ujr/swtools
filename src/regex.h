#pragma once
#ifndef REGEX_H
#define REGEX_H

size_t makepat(const char *s, int delim, strbuf *pat);
int match(const char *line, const char *pat);
int amatch(const char *line, int i, const char *pat, int j);

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

#endif
