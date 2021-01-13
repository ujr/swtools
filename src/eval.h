#pragma once
#ifndef EVAL_H
#define EVAL_H

enum { EVAL_OK = 0, EVAL_SYNTAX, EVAL_DIVZERO };

int evalint(const char *s, int *pval, const char **pmsg);

#endif
