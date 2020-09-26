#pragma once
#ifndef REGEX_H
#define REGEX_H

int match(const char *line, const char *pat);
int amatch(const char *line, int i, const char *pat, int j);

#endif
