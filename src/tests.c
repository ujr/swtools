/* Test Suite */

#include <stdio.h>

#include "tests.h"

#define UNUSED(x) (void)(x)

extern int strbuf_test(int *numpass, int *numfail);
extern int sorting_test(int *numpass, int *numfail);

int
main(int argc, char **argv)
{
  int numpass, numfail;

  UNUSED(argc);
  UNUSED(argv);

  int count_pass = 0;
  int count_fail = 0;

  numpass = numfail = 0;
  sorting_test(&numpass, &numfail);
  count_pass += numpass;
  count_fail += numfail;

  numpass = numfail = 0;
  strbuf_test(&numpass, &numfail);
  count_pass += numpass;
  count_fail += numfail;

  printf("%d fail, %d pass\n", count_fail, count_pass);
  return count_fail > 0;
}
