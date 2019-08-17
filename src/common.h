#include <errno.h>
#include <stdbool.h>  /* C99 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

#define VERSION "0.1"
#define RELEASE "0.1.0"

#define SUCCESS    0  /* status code for successful execution */
#define FAILHARD 127  /* status code for permanent (hard) error */
#define FAILSOFT 111  /* status code for temporary (soft) error */

#define SHIFTARGS(ac, av, n) do { (ac)-=n; (av)+=n; } while (0)
#define UNUSED(x) (void)(x)  /* to suppress "unused parameter" warnings */

typedef int toolfun(int argc, char **argv);

const char *me;  /* for error messages */
int verbosity;   /* normally 0=silenty */

/* Primitives */

#define putch(c) putchar(c)
#define putstr(s) fputs(s, stdout)
#define getch getchar

/* Utilities */

#define streq(s,t) (0==strcmp((s),(t)))

const char *getprog(char **argv);
int scanint(const char *s, int *v);
FILE *openin(const char *filepath);
int getline(strbuf *sp, int delim, FILE *fp);
int getln(char **buf, size_t *len, FILE *fp);
void printerr(const char *msg);
int checkioerr();

/* Command entry points */

int copycmd(int argc, char **argv);
int countcmd(int argc, char **argv);
int detabcmd(int argc, char **argv);
int entabcmd(int argc, char **argv);
int echocmd(int argc, char **argv);
int translitcmd(int argc, char **argv);
int comparecmd(int argc, char **argv);
int includecmd(int argc, char **argv);
