/* macro */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "strbuf.h"

#define BUF_ABORT nomem()
#include "buf.h"

#define HASHSIZE 53    /* prime */
#define ARGFLAG  '$'   /* macro argument character */
#define LPAREN   '('
#define COMMA    ','
#define RPAREN   ')'

typedef enum { UNDEF, MACTYPE, DEFINE, FORGET, IFDEF, DNL, DUMPDEFS } sttype;

/* symbol table entry */
struct ndblock {
  size_t nameofs;      /* offset of name in symbuf */
  size_t defnofs;      /* offset of defn in symbuf */
  sttype kind;         /* symbol kind: macro or one of the built-ins */
  struct ndblock *next;
};

/* call stack entry */
struct frame {
  sttype kind;         /* macro/built-in kind of this call */
  int plev;            /* number of open parens at this stack frame */
  size_t argp;         /* argument stack pointer */
};

static void expand(FILE *fp);
static void eval(sttype kind, int i, int j);
static void dodef(int i, int j);
static void doundef(int i, int j);
static void doifdef(int i, int j);
static void dodnl(void);
static int gettok(strbuf *sp, FILE *fp);
static void skipbl(FILE *fp);
static void unquote(strbuf *bp, FILE *fp);
/* TODO possible opts: hashsize, recursion/iteration limit */
static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

/* Evaluation Stack */

static void pushframe(int tt, int plev, size_t ap);
static struct frame popframe(void);
static bool inmac(void);
static void pushstr(const char *s);
static void pushnul(void);
static void poptoks(size_t ep);
static void pusharg(void);
static void poparg(size_t ap);

static void traceeval(sttype kind, int argstk[], int i, int j);

/* Symbol Table */

static sttype lookup(const char *pname, const char **pptext);
static void install(const char *pname, const char *ptext, sttype tt);
static void forget(const char *pname);
static void dumpsyms(void);
static void hashinit(void);
static void hashfree(void);
static struct ndblock *hashfind(const char *s);
static int hash(const char *s);

/* Push Back Input */

static int getpbc(FILE *fp);
static void unputc(char c);
static void unputs(const char *s);

/* Globals */

static char *pushbuf = 0;      /* push back buffer (buf.h) */
static strbuf tokbuf = {0};    /* token input buffer */
static strbuf symbuf = {0};    /* symbol definitions */
static struct ndblock *hashtab[HASHSIZE];

static struct frame *callstk = 0; /* macro call stack (buf.h) */
static int *argstk = 0;        /* indices into evalstk (buf.h) */
static char *evalstk = 0;      /* macro name, defn, arguments (buf.h) */

#define ARGSTR(k) (&evalstk[argstk[k]])  /* arg string at index k */

static char lquote = '`';
static char rquote = '\'';

int
macrocmd(int argc, char **argv)
{
  int r;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  hashinit();
  install("define", 0, DEFINE);
  install("forget", 0, FORGET);
  install("ifdef", 0, IFDEF);
  install("dnl", 0, DNL);
  install("dumpdefs", 0, DUMPDEFS);

  r = SUCCESS;
  if (argc > 0) while (*argv) {
    const char *fn = *argv++;
    FILE *fp = openin(fn);
    if (!fp) { r = FAILSOFT; goto done; }
    expand(fp);
    fclose(fp);
  }
  else expand(stdin);

  debug("{symbuf size: %ld}", strbuf_len(&symbuf));
done:
  hashfree();
  buf_free(pushbuf);
  strbuf_free(&tokbuf);
  strbuf_free(&symbuf);
  buf_free(callstk);
  buf_free(argstk);
  buf_free(evalstk);
  return r;
}

/* expand: define and expand macros in the given file */
static void
expand(FILE *fp)
{
  int c, tt;
  const char *token;
  const char *defn;

  while ((c = gettok(&tokbuf, fp)) != EOF) {
    token = strbuf_ptr(&tokbuf);
    if (isalpha(c)) {
      if ((tt = lookup(token, &defn)) == UNDEF) {
        pushstr(token); /* undefined token: emit or stack */
      }
      else { /* macro or builtin: start arg collection */
        pushframe(tt, 0, buf_size(argstk));
        /* push definition */
        pusharg();
        pushstr(defn);
        pushnul();
        /* push name (accessible as $0) */
        pusharg();
        pushstr(token);
        pushnul();
        /* start first arg */
        pusharg();
        /* peek at next token and fake () for nullary macro call */
        tt = gettok(&tokbuf, fp);
        unputs(strbuf_ptr(&tokbuf));
        if (tt != LPAREN) {
          unputc(RPAREN);
          unputc(LPAREN);
        }
      }
    }
    else if (c == lquote) {
      unquote(&tokbuf, fp);
    }
    else if (!inmac()) {
      putstr(token);
    }
    /* from here on we know callstk is not empty */
    else if (c == LPAREN) {
      struct frame *prec = buf_top(callstk);
      if (prec->plev > 0)
        pushstr(token);
      prec->plev += 1;
    }
    else if (c == RPAREN) {
      struct frame *prec = buf_top(callstk);
      prec->plev -= 1;
      if (prec->plev > 0)
        pushstr(token);
      else { /* end of arg list */
        size_t ap = buf_size(argstk);
        pushnul(); /* terminate last arg string */
        eval(prec->kind, prec->argp, ap-1);
        /* pop eval stack */
        struct frame rec = popframe();
        size_t ep = argstk[rec.argp];
        poparg(rec.argp);
        poptoks(ep);
      }
    }
    else if (c == COMMA && buf_top(callstk)->plev == 1) {
      pushnul();   /* finish previous arg */
      pusharg();   /* and start a new one */
      skipbl(fp);  /* ignore blanks after comma */
    }
    else {
      pushstr(token);  /* just stack it */
    }
  }

  if (inmac())
    error("unexpected end of input");
}

/* eval: expand args i..j: do built-in or push back defn */
static void
eval(sttype kind, int i, int j)
{
  if (verbosity > 1)
    traceeval(kind, argstk, i, j);
  if (kind == DEFINE)
    dodef(i, j);
  else if (kind == FORGET)
    doundef(i, j);
  else if (kind == IFDEF)
    doifdef(i, j);
  else if (kind == DNL)
    dodnl();
  else if (kind == DUMPDEFS)
    dumpsyms();
  else {
    int t = argstk[i];
    int k = t;
    while (evalstk[k] != '\0') k += 1;
    k -= 1;  /* last char of defn */
    while (k > t) {
      if (evalstk[k-1] != ARGFLAG)
        unputc(evalstk[k]);
      else {
        int argno = evalstk[k] - '0';
        if (0 <= argno && argno < j-i) {
          const char *a = ARGSTR(i+1+argno);
          unputs(a); /* push back argN in place of $N */
        }
        k -= 1;  /* skip over $ */
      }
      k -= 1;
    }
    if (k == t)  /* do last char */
      unputc(evalstk[k]);
  }
}

/* dodef: install definition in table */
static void dodef(int i, int j)
{
  if (j - i > 2) {
    const char *name = ARGSTR(i+2);
    const char *defn = ARGSTR(i+3);
    debug("{define %s=%s}", name, defn);
    install(name, defn, MACTYPE);
  }
  else error("%s: too few arguments", ARGSTR(i+1));
}

/* doundef: forget macro definition */
static void doundef(int i, int j)
{
  if (j - i > 1) {
    const char *name = ARGSTR(i+2);
    debug("{forget %s}", name);
    forget(name);
  }
  else error("%s: too few arguments", ARGSTR(i+1));
}

/* doifdef: expand $2 if $1 is defined, otherwise $3 */
static void doifdef(int i, int j)
{
  const char *text;
  if (j - i > 1) {
    const char *name = ARGSTR(i+2);
    sttype tt = lookup(name, &text);
    if (tt == UNDEF) {
      text = (j-i > 3) ? ARGSTR(i+4) : "";
    }
    else {
      text = (j-i > 2) ? ARGSTR(i+3) : "";
    }
    /* silently emit nothing if the respective arg is missing */
    debug("{ifdef %s (%s): %s}", name, tt == UNDEF ? "false" : "true", text);
    unputs(text);
  }
  else error("%s: too few arguments", ARGSTR(i+1));
}

/* dodnl: delete characters up to and including next newline */
static void dodnl(void)
{
  int c;
  do c = gettok(&tokbuf, stdin);
  while (c != '\n' && c != EOF);
}

/* unquote: strip quotes and put on output or eval stack */
static void unquote(strbuf *bp, FILE *fp)
{
  int level = 1;
  do {
    int c = gettok(bp, fp);
    if (c == rquote)
      level -= 1;
    else if (c == lquote)
      level += 1;
    else if (c == EOF)
      fatal("missing right quote");
    if (level > 0)
      pushstr(strbuf_ptr(bp));
  } while (level > 0);
}

/* gettok: read alnum sequence or a single non-alnum */
static int gettok(strbuf *sp, FILE *fp)
{
  int c = getpbc(fp);
  if (c == EOF) return EOF;
  strbuf_trunc(sp, 0);
  while (isalnum(c)) {
    strbuf_addc(sp, c);
    c = getpbc(fp);
  }
  if (strbuf_len(sp) > 0) {
    unputc(c); /* got one too far */
    return strbuf_char(sp, 0);
  }
  strbuf_addc(sp, c);
  return c; /* single non-alnum char */
}

/* skipbl: skip over blanks and tabs */
static void skipbl(FILE *fp)
{
  char c;
  do c = getpbc(fp);
  while (c == ' ' || c == '\t');
  unputc(c); /* went one too far */
}

/** Evaluation Stack **/

static void pushframe(int tt, int plev, size_t ap) {
  struct frame rec;
  rec.kind = tt;
  rec.plev = plev;
  rec.argp = ap;
  buf_push(callstk, rec);
}

static struct frame popframe(void) {
  assert(buf_size(callstk) > 0);
  return buf_pop(callstk);
}

static bool inmac(void) {
  return buf_size(callstk) > 0;
}

static void pushstr(const char *s) {
  if (inmac()) while (*s) {
    buf_push(evalstk, *s++);
  }
  else putstr(s);
}

static void pushnul(void) {
  assert(inmac());
  buf_push(evalstk, '\0');
}

static void poptoks(size_t ep) {
  while (buf_size(evalstk) > ep)
    (void) buf_pop(evalstk);
  //dumpstack();
}

static void pusharg(void) {
  size_t ss = buf_size(evalstk);
  buf_push(argstk, ss);
}

static void poparg(size_t ap) {
  while (buf_size(argstk) > ap)
    (void) buf_pop(argstk);
}

#if 0
static void dumpstack(void) {
  size_t n;
  fprintf(stderr, "Eval stack (cp=%zu, ap=%zu, ep=%zu):\n",
          buf_size(callstk), buf_size(argstk), buf_size(evalstk));
  n = buf_size(argstk);
  while (n > 0) {
    n -= 1;
    fprintf(stderr, "%2zd:%s$\n", n, &evalstk[argstk[n]]);
  }
}
#endif

static void
traceeval(sttype kind, int argstk[], int i, int j) {
  int k;
  fprintf(stderr, "eval(kind=%d,i=%d,j=%d):\n", kind, i, j);
  for (k = j; k >= i; k--) {
    fprintf(stderr, "%2d:%s$\n", k, ARGSTR(k));
  }
}

/** Symbol Table **/

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
forget(const char *pname)
{
  int h = hash(pname);
  struct ndblock *p = hashtab[h];
  struct ndblock **q = &hashtab[h];
  /* unlink needs pointer to prev ndblock */

  while (p) {
    const char *s = strbuf_ptr(&symbuf) + p->nameofs;
    if (streq(s, pname)) {
      *q = p->next; /* unlink */
      free(p);
      return;
    }
    q = &(p->next);
    p = p->next;
  }
  /* silently not found */
}

/* dumpsyms: dump the symbol table to stderr */
static void
dumpsyms(void)
{
  int i;
  const char *base = strbuf_ptr(&symbuf);
  for (i = 0; i < HASHSIZE; i++) {
    struct ndblock *p = hashtab[i];
    while (p) {
      debug("{%2d:%s=%d,%s}", i, base+p->nameofs, p->kind, base+p->defnofs);
      p = p->next;
    }
  }
  debug("{hashsize is %d}", HASHSIZE);
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

/** Push Back Input **/

static int
getpbc(FILE *fp)
{
  return buf_size(pushbuf) > 0 ? buf_pop(pushbuf) : getc(fp);
}

static void
unputc(char c)
{
  buf_push(pushbuf, c);
}

static void
unputs(const char *s)
{
  size_t len = strlen(s);
  while (len > 0) unputc(s[--len]);
}

/** Miscellaneous **/

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
  fprintf(fp, "Macro processor: expand string definitions with arguments.\n");
  fprintf(fp, "Define macros: define(name,expansion) where `expansion` may\n");
  fprintf(fp, "refer to arguments $1 to $9, e.g., define(putc,fputc($1,STDOUT))\n");
}
