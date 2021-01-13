/* Unit tests for evalint.c */

#include <stdio.h>

#include "test.h"
#include "eval.h"

void
eval_test(int *pnumpass, int *pnumfail)
{
  int numpass = 0;
  int numfail = 0;

  int value;
  const char *msg;

  HEADING("Testing evalint.c");

  TEST("1+2*3", evalint("1+2*3", &value, &msg) == EVAL_OK && value == 7);
  TEST(" ( 1 + 2 ) * 3 ", evalint(" ( 1 + 2 ) * 3 ", &value, &msg) == EVAL_OK && value == 9);
  TEST(" -42 ", evalint(" -42 ", &value, &msg) == EVAL_OK && value == -42);
  TEST("-5--2", evalint("-5--2", &value, &msg) == EVAL_OK && value == -3);
  TEST("100/5/4", evalint("100/5/3", &value, &msg) == EVAL_OK && value == 6);

  TEST("null", evalint(0, 0, 0) == EVAL_SYNTAX);
  TEST("empty", evalint("", &value, &msg) == EVAL_SYNTAX);
  INFO("expected: error: %s", msg);
  TEST("5/0", evalint("5/0", &value, &msg) == EVAL_DIVZERO);
  INFO("expected: error: %s", msg);
  TEST("2*(3-)", evalint("2*(3-)", &value, &msg) == EVAL_SYNTAX);
  INFO("expected: error: %s", msg);
  TEST("2*(3-1", evalint("2*(3-1", &value, &msg) == EVAL_SYNTAX);
  INFO("expected: error: %s", msg);

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}
