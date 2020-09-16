/* Test Runner */

#include <stdio.h>

#include "test.h"

#define UNUSED(x) (void)(x)

extern void buf_test(int *pnumpass, int *pnumfail);
extern void strbuf_test(int *pnumpass, int *pnumfail);
extern void sorting_test(int *pnumpass, int *pnumfail);

const char *me = "runtests";

int
main(int argc, char **argv)
{
  int numpass = 0;
  int numfail = 0;

  UNUSED(argc);
  UNUSED(argv);

  buf_test(&numpass, &numfail);
  strbuf_test(&numpass, &numfail);
  sorting_test(&numpass, &numfail);

  SUMMARY(numpass, numfail);
  return numfail > 0 ? 1 : 0;
}
