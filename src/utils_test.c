/* Unit tests for utils.c */

#include <stdio.h>

#include "test.h"
#include "common.h"

void
utils_test(int *pnumpass, int *pnumfail)
{
  int numpass = 0;
  int numfail = 0;

  int i;
  const char *s;
  strbuf sb = {0};

  HEADING("Testing escape()");
  s = "a\\n\\t\\\\\\x\\";
  i = 0;
  TEST("escape a", escape(s, &i) == 'a' && i == 1);
  TEST("escape \\n", escape(s, &i) == '\n' && i == 3);
  TEST("escape \\t", escape(s, &i) == '\t' && i == 5);
  TEST("escape \\\\", escape(s, &i) == '\\' && i == 7);
  TEST("escape \\x", escape(s, &i) == 'x' && i == 9);
  TEST("escape \\$", escape(s, &i) == '\\' && i == 10);

  HEADING("Testing scanstr()");
  strbuf_trunc(&sb, 0);
  TEST("scanstr null", scanstr(0, &sb) == 0);
  TEST("scanstr empty", scanstr("", &sb) == 0);
  TEST("scanstr \"\"", scanstr("\"\"", &sb) == 2);
  TEST("scanstr \"abc\"", scanstr("\"abc\"", &sb) == 5);
  TEST("scanstr escapes", scanstr("\"\\t\\n\\\"\\x\"", &sb) == 10);
  TEST("buffer so far", STREQ("abc\t\n\"x", strbuf_ptr(&sb)));

  TEST("unterminated 1", scanstr("\"unterminated", &sb) == 0);
  TEST("unterminated 2", scanstr("\"unterminated\\\"", &sb) == 0);
  TEST("control char", scanstr("\"cntrl\nchar\"", &sb) == 0);
  TEST("esc at end", scanstr("\"esc at end\\", &sb) == 0);

  HEADING("Testing pathqualify()");
  TEST("null null", streq(pathqualify(&sb, 0, 0), ""));
  TEST("file null", streq(pathqualify(&sb, "file.txt", 0), "file.txt"));
  TEST("file file", streq(pathqualify(&sb, "file.txt", "base.txt"), "file.txt"));
  TEST("file dir", streq(pathqualify(&sb, "file.txt", "foo/bar"), "foo/file.txt"));
  TEST("dir/file dir", streq(pathqualify(&sb, "dir/file.txt", "foo/bar"), "foo/dir/file.txt"));
  TEST("/dir/file dir", streq(pathqualify(&sb, "/dir/file.txt", "foo/bar"), "/dir/file.txt"));

  HEADING("Testing dodash()");
  strbuf_trunc(&sb, 0);
  TEST("dodash", dodash("-ab-de-", 0, '\0', &sb) == 7 && STREQ("-abcde-", strbuf_ptr(&sb)));
  strbuf_trunc(&sb, 0);
  TEST("dodash", dodash("a\\-c\\te-d", 0, '\0', &sb) == 9 && STREQ("a-c\te-d", strbuf_ptr(&sb)));

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}
