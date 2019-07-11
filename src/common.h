#include <stdio.h>

#define VERSION "0.1"
#define RELEASE "0.1.0"

#define SUCCESS    0  /* status code for successful execution */
#define FAILHARD 127  /* status code for permanent (hard) error */
#define FAILSOFT 111  /* status code for temporary (soft) error */

#define streq(s,t) (0==strcmp((s),(t)))
#define putch(c) putchar(c)
#define putstr(s) fputs(s, stdout)

typedef int toolfun(int argc, char **argv);

/* Command entry points */

int echocmd(int argc, char **argv);

/* Utilities */

const char *progname(char **argv);
