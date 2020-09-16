/* Unit tests for strbuf.{c,h} */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test.h"
#include "strbuf.h"

#define LEN(sp)   strbuf_len(sp)
#define SIZE(sp)  strbuf_size(sp)
#define LENLTSIZE(sp)  (LEN(sp) < SIZE(sp))
#define TERMINATED(sp) (0 == (sp)->buf[LEN(sp)])
#define INVARIANTS(sp) (LENLTSIZE(sp) && TERMINATED(sp))

void
strbuf_test(int *pnumpass, int *pnumfail)
{
  strbuf sb = {0};
  strbuf *sp = &sb;
  int i;

  int numpass = 0;
  int numfail = 0;

  HEADING("Testing strbuf.{c,h}");

  INFO("sizeof strbuf: %zu bytes", sizeof(strbuf));
  TEST("init len 0", strbuf_len(sp) == 0);

  strbuf_addb(sp, "Hellooo", 5);
  TEST("addb", STREQ(sp->buf, "Hello") && INVARIANTS(sp));
  strbuf_addc(sp, ',');
  strbuf_addc(sp, ' ');
  TEST("addc", STREQ(sp->buf, "Hello, ") && INVARIANTS(sp));
  strbuf_addz(sp, "World!");
  TEST("addz", STREQ(sp->buf, "Hello, World!") && INVARIANTS(sp));
  strbuf_trunc(sp, 5);
  TEST("trunc 5", STREQ(sp->buf, "Hello") && INVARIANTS(sp));
  strbuf_addf(sp, "+%d-%d=%s", 3, 4, "konfus");
  TEST("addf", STREQ(sp->buf, "Hello+3-4=konfus") && INVARIANTS(sp));
  sbchar(sp, 10) = 'Q';
  TEST("char", sbchar(sp, 10) == 'Q');
  strbuf_trunc(sp, 0);
  TEST("trunc 0 (buf)", STREQ(sp->buf, "") && INVARIANTS(sp));
  TEST("trunc 0 (len)", strbuf_len(sp) == 0 && INVARIANTS(sp));

  for (i = 0; i < 26; i++) {
    strbuf_addc(sp, "abcdefghijklmnopqrstuvwxyz"[i]);
    strbuf_addc(sp, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]);
  }
  TEST("52*addc", strbuf_len(sp) == 52 && INVARIANTS(sp) &&
     STREQ(sp->buf, "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ"));

  strbuf_trunc(sp, 0);
  TEST("trunc 0", strbuf_len(sp) == 0 && INVARIANTS(sp));
  strbuf_addb(sp, "\0\a\b\f\n\r\t\v\0", 9);
  TEST("embedded \\0", strbuf_len(sp) == 9 && INVARIANTS(sp));
  INFO("size=%zu", SIZE(sp));

  strbuf_free(sp);
  TEST("free", sp->buf == 0 && LEN(sp) == 0 && SIZE(sp) == 0);

  strbuf_addz(sp, "From Moby Dick:\n"); /* 1st alloc after free() */
  strbuf_addz(sp, /* will trigger an exact alloc */
"Call me Ishmael. Some years ago—never mind how long precisely—having "
"little or no money in my purse, and nothing particular to interest me "
"on shore, I thought I would sail about a little and see the watery part "
"of the world. It is a way I have of driving off the spleen and "
"regulating the circulation. Whenever I find myself growing grim about "
"the mouth; whenever it is a damp, drizzly November in my soul; whenever "
"I find myself involuntarily pausing before coffin warehouses, and "
"bringing up the rear of every funeral I meet; and especially whenever "
"my hypos get such an upper hand of me, that it requires a strong moral "
"principle to prevent me from deliberately stepping into the street, and "
"methodically knocking people’s hats off—then, I account it high time to "
"get to sea as soon as I can. This is my substitute for pistol and ball. "
"With a philosophical flourish Cato throws himself upon his sword; I "
"quietly take to the ship. There is nothing surprising in this. If they "
"but knew it, almost all men in their degree, some time or other, "
"cherish very nearly the same feelings towards the ocean with me.");
  TEST("large addz (len+1==size)", LEN(sp)+1 == SIZE(sp) && INVARIANTS(sp));
  INFO("size=%zu, len=%zu", SIZE(sp), LEN(sp));
  strbuf_addc(sp, '\n');
  TEST("single addc (len+1<size)", LEN(sp)+1 < SIZE(sp) && INVARIANTS(sp));
  INFO("size=%zu, len=%zu", SIZE(sp), LEN(sp));
  TEST("not failed", !strbuf_failed(sp));

  strbuf_free(sp);

  strbuf_addz(sp, ""); /* length 0, must still trigger alloc for \0 */
  strbuf_addb(sp, "Hellooo", 5);
  strbuf_addc(sp, ' ');
  strbuf_addz(sp, "World!");
  strbuf_trunc(sp, 6);
  strbuf_addf(sp, "User #%d", 123);
  INFO("%s (len=%zu)", sp->buf, LEN(sp));
  TEST("sample", STREQ(sp->buf, "Hello User #123") && INVARIANTS(sp));

  strbuf_free(sp);

  /* Exercise allocation through addc(): */
  for (i = 0; i < 100*1024*1024; i++) {
    strbuf_addc(sp, "abcdefghijklmnopqrstuvwxyz"[i%26]);
  }
  TEST("100M*addc", strbuf_len(sp) == 100*1024*1024 && INVARIANTS(sp));
  INFO("size=%zu, len=%zu", SIZE(sp), LEN(sp));
  strbuf_free(sp);

  /* Exercise allocation through addb(): */
  for (i = 0; i < 5*1024*1024; i++) {
    strbuf_addb(sp, "01234567890123456789", 20);
  }
  TEST("5M*addb", strbuf_len(sp) == 100*1024*1024 && INVARIANTS(sp));
  INFO("size=%zu, len=%zu", SIZE(sp), LEN(sp));
  strbuf_free(sp);

  /* Exercise allocation through addf(): */
  for (i = 0; i < 5*1024*1024; i++) {
    strbuf_addf(sp, "appending fmt: %zu", (size_t) i);
  }
  TEST("5M*addf", INVARIANTS(sp));
  INFO("size=%zu, len=%zu", SIZE(sp), LEN(sp));
  strbuf_free(sp);

#if STRBUF_TEST_ALLOC_TILL_FAIL
  /* Allocating until memory failure: */
  while (!strbuf_failed(sp)) {
    strbuf_addz(sp, "Appending zero-terminated string until memory failure\n");
  }
  TEST("alloc until failure", INVARIANTS(sp));
  INFO("size=%zu, len=%zu, state is failed", SIZE(sp), LEN(sp));
  strbuf_free(sp);
#endif

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}
