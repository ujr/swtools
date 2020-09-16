/* test.h - utils for super simple unit tests
 *
 * Usage:
 *   int numpass = 0;  // incremented on each TEST passed
 *   int numfail = 0;  // incremented on each TEST failed
 *   HEADING("Basic Tests");
 *   TEST("can add", 2+3==5);
 *   TEST("can foo()", foo()==42);
 *   TEST("str fun", STREQ(bar(), "fun"));
 *   INFO("state is now %d", px->state);
 *   SUMMARY(numpass, numfail);
 */

#include <stdio.h>
#include <string.h>

#if _WIN32
# define TERMPASS  "PASS"
# define TERMFAIL  "FAIL"
# define TERMHEAD  "%s\n"
# define TERMINFO  "INFO"
# define TERMBAD   ""
# define TERMGOOD  ""
# define TERMRESET ""
#else /* not Windows: assume an ANSI terminal */
# define ANSIRED   "\033[31m"
# define ANSIGREEN "\033[32m"
# define ANSIBLUE  "\033[34m"
# define ANSICYAN  "\033[36m"
# define ANSIBOLD  "\033[1m"
# define ANSIRESET "\033[0m"
# define TERMBAD   ANSIBOLD ANSIRED
# define TERMGOOD  ANSIBOLD ANSIGREEN
# define TERMRESET ANSIRESET
# define TERMPASS  ANSIGREEN "PASS" ANSIRESET
# define TERMFAIL  ANSIRED   "FAIL" ANSIRESET
# define TERMHEAD  ANSIBOLD "%s" ANSIRESET "\n"
# define TERMINFO  ANSIBLUE "INFO" ANSIRESET
#endif

#define STREQ(s,t) (0==strcmp((s),(t)))

#define HEADING(s) printf(TERMHEAD, (s))

#define TEST(s, x) \
  do { \
    if (x) { \
      printf(TERMPASS " %s\n", (s)); \
      numpass++; \
    } else { \
      printf(TERMFAIL " %s\n", (s)); \
      numfail++; \
    } \
  } while (0)

#define INFO(fmt, ...) \
  printf(TERMINFO " " fmt "\n", __VA_ARGS__)

#define SUMMARY(npass, nfail) \
  do { \
    if (nfail > 0) \
      printf(TERMBAD "Oops: %d fail, %d pass" TERMRESET "\n", nfail, npass); \
    else \
      printf(TERMGOOD "OK: %d fail, %d pass" TERMRESET "\n", nfail, npass); \
  } while (0)
