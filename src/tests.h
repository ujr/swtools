/* tests.h - utils for unit tests
 *
 * Usage:
 *   int count_pass = 0;
 *   int count_fail = 0;
 *   HEADING("Basic Tests");
 *   TEST("can add", 2+3==5)
 *   TEST("can foo()", foo()==42)
 *   TEST("str fun", STREQ(bar(), "fun"))
 *   INFO("state is now %d", xp->state);
 */

#if _WIN32
#  define TERMPASS  "PASS"
#  define TERMFAIL  "FAIL"
#  define TERMHEAD  "%s\n"
#  define TERMINFO  "INFO"
#else
#  define ANSIRED   "\033[31m"
#  define ANSIGREEN "\033[32m"
#  define ANSIBLUE  "\033[34m"
#  define ANSICYAN  "\033[36m"
#  define ANSIBOLD  "\033[1m"
#  define ANSIRESET "\033[0m"
#  define TERMPASS  ANSIGREEN "PASS" ANSIRESET
#  define TERMFAIL  ANSIRED   "FAIL" ANSIRESET
#  define TERMHEAD  ANSIBOLD "%s" ANSIRESET "\n"
#  define TERMINFO  ANSIBLUE "INFO" ANSIRESET
#endif

#define STREQ(s,t) (0==strcmp((s),(t)))

#define HEADING(s) printf(TERMHEAD, (s))

#define TEST(s, x) \
  do { \
    if (x) { \
      printf(TERMPASS " %s\n", (s)); \
      ++count_pass; \
    } else { \
      printf(TERMFAIL " %s\n", (s)); \
      ++count_fail; \
    } \
  } while (0)

#define INFO(fmt, ...) \
  printf(TERMINFO " " fmt "\n", __VA_ARGS__)

