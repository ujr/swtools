/* Unit tests for regex.c */

#include <stdio.h>

#include "test.h"
#include "regex.h"
#include "strbuf.h"

static int
Match(const char *line, const char *pat, int flags);

static const char *
Subst(const char *line, const char *pat, const char *rep, strbuf *buf);

void
regex_test(int *pnumpass, int *pnumfail)
{
  int numpass = 0;
  int numfail = 0;

  size_t s;
  strbuf patbuf = {0};
  strbuf outbuf = {0};
  const char *pat;

  HEADING("Testing regex.c");

  pat = "^[ \\t]*a+b*.*$";
  s = makepat(pat, '\0', &patbuf);
  TEST("makepat", s == 14 && STREQ("^*[\002 \t+ca*cb*.$", strbuf_ptr(&patbuf)));
  TEST("match a", Match("a", pat, 0) == 0);
  TEST("match aabbcc", Match(" aabbcc", pat, 0) == 0);
  TEST("match \\tabb$", Match("\tabb$", pat, 0) == 0);
  TEST("match \\tx", Match("\txabc", pat, 0) < 0);


  pat = "a^b$c";
  strbuf_trunc(&patbuf, 0);
  s = makepat(pat, '\0', &patbuf);
  TEST("makepat a^b$c", s == 5 && STREQ("cac^cbc$cc", strbuf_ptr(&patbuf)));
  TEST("match xa^b$cz", Match("xa^b$cz", pat, 0) == 1);
  TEST("match xA^B$Cz", Match("xA^B$Cz", pat, regex_none) < 0);
  TEST("match xA^B$Cz", Match("xA^B$Cz", pat, regex_ignorecase) == 1);

  /* \n in input is needed for correct result; in change(1) this is guaranteed */
  TEST("subst xy/a*/\\&", STREQ("&x&y&\n", Subst("xy\n", "a*", "\\&", &outbuf)));
  TEST("subst xay/a*/\\&", STREQ("&x&y&\n", Subst("xay\n", "a*", "\\&", &outbuf)));
  TEST("subst xay/a*/&", STREQ("xay\n", Subst("xay\n", "a*", "&", &outbuf)));

  strbuf_free(&patbuf);
  strbuf_free(&outbuf);

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}

static int
Match(const char *line, const char *pat, int flags)
{
  int r;
  strbuf patbuf = {0};
  makepat(pat, '\0', &patbuf);
  r = match(line, strbuf_ptr(&patbuf), flags);
  strbuf_free(&patbuf);
  return r;
}

static const char *
Subst(const char *line, const char *pat, const char *rep, strbuf *buf)
{
  strbuf patbuf = {0};
  makepat(pat, '\0', &patbuf);
  strbuf_trunc(buf, 0);
  subline(line, strbuf_ptr(&patbuf), 0, rep, buf);
  strbuf_free(&patbuf);
  return strbuf_ptr(buf);
}
