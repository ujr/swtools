
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "strbuf.h"

static void nomem(void)
{
  printerr("out of memory");
  longjmp(errjmp, 1);
}

#define BUF_ABORT nomem()
#include "buf.h"

#define STR(sp) strbuf_ptr(sp)
#define HASHSIZE 53  /* prime */

typedef enum { UNDEF, DEFTYPE, MACTYPE } sttype;

struct ndblock {
  size_t nameofs;
  size_t defnofs;
  sttype kind;
  struct ndblock *next;
};

static int gettok(strbuf *sp, FILE *fp);
static void getdef(strbuf *pname, strbuf *ptext, FILE *fp);

static sttype lookup(const char *pname, const char **pptext);
static void install(const char *pname, const char *ptext, sttype tt);
static void hashinit(void);
static void hashfree(void);
static struct ndblock *hashfind(const char *s);
static int hash(const char *s);

static int getpbc(FILE *fp);
static void putback(char c);
static void pbstr(const char *s);
static void skipbl(FILE *fp);

static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

static char *pushbuf = 0; /* push back (buf.h) */
static strbuf symbuf = {0}; /* symbol definitions */
static struct ndblock *hashtab[HASHSIZE];

int
definecmd(int argc, char **argv)
{
  int r, c, tt;
  strbuf tokbuf = {0};
  strbuf defbuf = {0};
  const char *token;
  const char *defn;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  hashinit();
  install("define", 0, DEFTYPE);
  while ((c = gettok(&tokbuf, stdin)) != EOF) {
    token = strbuf_ptr(&tokbuf);
    if (!isalpha(c))
      putstr(token);
    else if ((tt = lookup(token, &defn)) == UNDEF)
      putstr(token);
    else if (tt == DEFTYPE) {
      getdef(&tokbuf, &defbuf, stdin);
      install(STR(&tokbuf), STR(&defbuf), MACTYPE);
    }
    else
      pbstr(STR(&defbuf));
  }

  hashfree();
  strbuf_free(&tokbuf);
  strbuf_free(&defbuf);
  buf_free(pushbuf);
  return SUCCESS;
}

/* gettok: read alnum sequence or a single non-alnum */
static int
gettok(strbuf *sp, FILE *fp)
{
  int c = getpbc(fp);
  if (c == EOF) return EOF;
  strbuf_trunc(sp, 0);
  while (isalnum(c)) {
    strbuf_addc(sp, c);
    c = getpbc(fp);
  }
  if (strbuf_len(sp) > 0) {
    putback(c); /* got one too far */
    return strbuf_char(sp, 0);
  }
  strbuf_addc(sp, c);
  return c; /* single non-alnum char */
}

/* getdef: read "(name, \s*, balanced)" */
static void
getdef(strbuf *pname, strbuf *ptext, FILE *fp)
{
  int c, nlpar;
  strbuf_trunc(pname, 0);
  strbuf_trunc(ptext, 0);
  if (getpbc(fp) != '(')
    message("{define: missing left paren}");
  else if (!isalpha(gettok(pname, fp)))
    message("{define: non-alphanumeric macro name}");
  else if (getpbc(fp) != ',')
    message("{define: missing comma after macro name}");
  else {
    skipbl(fp);
    nlpar = 1;
    while (nlpar > 0) {
      c = getpbc(fp);
      if (c == EOF)
        fatal("define: end-of-input while reading definition");
      strbuf_addc(ptext, c);
      if (c == '(') nlpar++;
      else if (c == ')') nlpar--;
    }
    strbuf_trunc(ptext, strbuf_len(ptext) - 1); /* omit closing paren */
  }
}

static sttype
lookup(const char *pname, const char **pptext)
{
  struct ndblock *p = hashfind(pname);
  if (!p) return UNDEF;
  *pptext = strbuf_ptr(&symbuf) + p->defnofs;
  return p->kind;
}

static void
install(const char *pname, const char *ptext, sttype tt)
{
  struct ndblock *ndptr;
  int h = hash(pname);

  ndptr = malloc(sizeof(*ndptr));
  if (!ndptr) nomem();

  ndptr->next = hashtab[h];
  hashtab[h] = ndptr;

  ndptr->nameofs = strbuf_len(&symbuf);
  strbuf_addz(&symbuf, pname ? pname : "");
  strbuf_addc(&symbuf, '\0');
  ndptr->defnofs = strbuf_len(&symbuf);
  strbuf_addz(&symbuf, ptext ? ptext : "");
  strbuf_addc(&symbuf, '\0');
  ndptr->kind = tt;
}

static void
hashinit(void)
{
  int i;
  for (i = 0; i < HASHSIZE; i++)
    hashtab[i] = 0;
}

static void
hashfree(void)
{
  struct ndblock *p, *q;
  int i;
  for (i = 0; i < HASHSIZE; i++) {
    for (p = hashtab[i]; p; p = q) {
      q = p->next;
      free(p);
    }
  }
  strbuf_free(&symbuf);
}

static struct ndblock *
hashfind(const char *s)
{
  const char *pname;
  struct ndblock *p;
  for (p = hashtab[hash(s)]; p; p  = p->next) {
    pname = strbuf_ptr(&symbuf) + p->nameofs;
    if (streq(pname, s)) return p;
  }
  return 0; /* not found */
}

static int
hash(const char *s)
{
  int h;
  for (h = 0; *s; s++)
    h = (3*h + *s) % HASHSIZE;
  return h;
}


static int getpbc(FILE *fp)
{
  if (buf_size(pushbuf) > 0)
    return buf_pop(pushbuf);
  return getc(fp);
}

static void putback(char c)
{
  buf_push(pushbuf, c);
}

static void pbstr(const char *s)
{
  size_t len = strlen(s);
  while (len > 0) putback(s[--len]);
}

static void skipbl(FILE *fp)
{
  char c;
  do c = getpbc(fp);
  while (c == ' ' || c == '\t');
  putback(c); /* went one too far */
}

static int
parseopts(int argc, char **argv)
{
  int i, showhelp = 0;
  for (i = 1; i <= argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': showhelp = 1; break;
        default: usage("invalid option");
          return -1;
      }
    }
  }
  if (showhelp) {
    usage(0);
    exit(SUCCESS);
  }
  return i; /* #args parsed */
}

static void
usage(const char *errmsg)
{
  FILE *fp = errmsg ? stderr : stdout;
  if (errmsg) fprintf(fp, "%s: %s\n", me, errmsg);
  fprintf(fp, "Usage: %s [files]\n", me);
  fprintf(fp, "Expand string definitions\n");
}
