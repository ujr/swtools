/* Unit tests for utils.c */

#include <stdio.h>

#include "test.h"
#include "common.h"

void
utils_test(int *pnumpass, int *pnumfail)
{
  int numpass = 0;
  int numfail = 0;

  size_t i;
  const char *s;
  strbuf sb = {0};

  HEADING("Testing escape()");
  s = "a\\n\\t\\";
  i = 0;
  TEST("escape a", escape(s, &i) == 'a' && i == 1);
  TEST("escape \\n", escape(s, &i) == '\n' && i == 3);
  TEST("escape \\t", escape(s, &i) == '\t' && i == 5);
  TEST("escape \\$", escape(s, &i) == '\\' && i == 6);

  HEADING("Testing dodash()");
  strbuf_trunc(&sb, 0);
  TEST("dodash", dodash("-ab-de-", 0, '\0', &sb) == 7 && STREQ("-abcde-", strbuf_ptr(&sb)));
  strbuf_trunc(&sb, 0);
  TEST("dodash", dodash("a\\-c\\te-d", 0, '\0', &sb) == 9 && STREQ("a-c\te-d", strbuf_ptr(&sb)));

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}
