#include <errno.h>
#include <setjmp.h>
#include <stdbool.h>  /* C99 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

#define VERSION "0.1.0"

#define SUCCESS    0  /* status code for successful execution */
#define FAILHARD 127  /* status code for permanent (hard) error */
#define FAILSOFT 111  /* status code for temporary (soft) error */

#define SHIFTARGS(ac, av, n) do { (ac)-=n; (av)+=n; } while (0)
#define UNUSED(x) (void)(x)  /* to suppress "unused parameter" warnings */

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

#define ESC '\\'
#define DASH '-'

typedef int toolfun(int argc, char **argv);

extern const char *me;  /* for error messages */
extern int verbosity;   /* normally 0=silent */
extern jmp_buf errjmp;  /* longjmp here to give up */

/* Primitives */

#define putch(c) putchar(c)
#define putcf(c,fp) fputc(c,fp)
#define putstr(s) fputs(s, stdout)
#define getch getchar
#define getcf(fp) fgetc(fp)

#define streq(s,t) (0==strcmp((s),(t)))

/* utils.c */

const char *getprog(char **argv);
size_t scanint(const char *s, int *v);
size_t scanspace(const char *s);
void skipblank(const char *s, int *pidx);
size_t scanstr(const char *s, strbuf *sp);
char escape(const char *s, int *pi);
int dodash(const char *s, int i, char delim, strbuf *buf);
FILE *openin(const char *filepath);
FILE *openout(const char *filepath);
void filecopy(FILE *fin, FILE *fout);
int getline(strbuf *sp, int delim, FILE *fp);
int getln(char **buf, size_t *len, FILE *fp);
char *strclone(const char *s);
int checkioerr();

void message(const char *fmt, ...); /* write line to stdout */
void debug(const char *fmt, ...); /* write line to stderr, if verbose */
void error(const char *fmt, ...); /* always write line to stderr */
void fatal(const char *fmt, ...); /* write line to stderr and bail out */
void nomem(void); /* write "out of memory" to stderr and bail out */

/* translit.c */

int index(const char *s, size_t slen, int c);
int xindex(bool allbut, const char *s, size_t slen, int c, int lastdst);

/* Command entry points */

int copycmd(int argc, char **argv);
int countcmd(int argc, char **argv);
int detabcmd(int argc, char **argv);
int entabcmd(int argc, char **argv);
int echocmd(int argc, char **argv);
int translitcmd(int argc, char **argv);
int comparecmd(int argc, char **argv);
int includecmd(int argc, char **argv);
int concatcmd(int argc, char **argv);
int printcmd(int argc, char **argv);
int sortcmd(int argc, char **argv);
int uniquecmd(int argc, char **argv);
int shufflecmd(int argc, char **argv);
int findcmd(int argc, char **argv);
int changecmd(int argc, char **argv);
int editcmd(int argc, char **argv);
int definecmd(int argc, char **argv);
int macrocmd(int argc, char **argv);
