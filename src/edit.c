
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "common.h"
#include "regex.h"
#include "strbuf.h"

static void debug(const char *fmt, ...);
static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

#define MIN(x,y) ((x)<(y)?(x):(y))

#define ACMD     'a'    /* append (after current line) */
#define CCMD     'c'    /* change (replace) line(s) */
#define DCMD     'd'    /* delete */
#define ICMD     'i'    /* insert (before current line) */
#define JCMD     'j'    /* join lines */
#define MCMD     'm'    /* move line(s) */
#define PCMD     'p'    /* print */
#define QCMD     'q'    /* quit */
#define SCMD     's'    /* substitute */
#define EQCMD    '='    /* show line number */

#define NEWLINE  '\n'
#define CURLINE  '.'
#define LASTLINE '$'
#define SCAN     '/'
#define SCANBACK '\\'

typedef struct {
  const char *z;
  bool mark;
} bufitem;

typedef struct {
  int line1;  /* first line number */
  int line2;  /* second line number */
  int nlines; /* total # of line numbers specified */
  int curln;  /* current line: value of dot */
  int lastln; /* last line: value of $ */
  strbuf patbuf; /* search pattern */
  strbuf subbuf; /* substitute text */
  strbuf linebuf; /* for line editing */
  bufitem *buffer;
  FILE *fin;  /* input commands */
} edstate;

/* status of operations: returned by most functions */
typedef enum { ED_OK, ED_ERR, ED_END } opstate;

/* getting the line numbers and command arguments */
static opstate getlist(edstate *ped, const char *line, int *pi);
static opstate getone(edstate *ped, const char *line, int *pi, int *pnum);
static opstate getnum(edstate *ped, const char *line, int *pi, int *pnum);
static opstate optpat(edstate *ped, const char *line, int *pi);
static opstate patscan(edstate *ped, char direction, int *pnum);
static opstate getsub(edstate *ped, const char *cmd, int *pi, bool *gflag);
static opstate defaultlines(edstate *ped, int def1, int def2);
static int nextln(edstate *ped, int n);
static int prevln(edstate *ped, int n);

/* the line buffer interface */
static void bufinit(edstate *ped); /* init buffer (only line zero, scratch file) */
static void buffree(edstate *ped); /* release resources (memory, scratch file, whatever) */
static bufitem makebuf(const char *z);
static void reverse(bufitem *buf, int n1, int n2); /* helper for bufmove */
static void bufmove(bufitem *buf, int n1, int n2, int n3); /* move lines n1..n2 after n3 */
static const char *bufget(edstate *ped, int n); /* get line n for editing */
static opstate bufput(edstate *ped, const char *line); /* add after curln */
static const char *copyline(const char *z); /* malloc'ed copy */
//static char getmark(edstate *ped, int n);
//static void putmark(edstate *ped, int n, char mark);

/* doing the commands */
static opstate docmd(edstate *ped, const char *cmd, int *pi, bool glob);
static opstate checkp(const char *cmd, int *pi, bool *ppflag);
static opstate doprint(edstate *ped, int n1, int n2);
static opstate doappend(edstate *ped, int n, bool glob);
static opstate dodelete(edstate *ped, int n1, int n2);
static opstate domove(edstate *ped, int n3);
static opstate dojoin(edstate *ped, int n1, int n2);
static opstate dosubst(edstate *ped, bool gflag, bool glob);
static void message(const char *msg);

int
editcmd(int argc, char **argv)
{
  const char *fn;
  const char *cmd;
  int r, i, cursave;
  strbuf cmdbuf = {0};
  edstate ed;
  opstate status;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  i = (int) strlen("abc");
  i = (int) strlen("x\n");

  fn = 0;
  if (argc > 0 && *argv) {
    argc--;
    fn = *argv++;
  }

  if (argc > 0 && *argv) {
    usage("too many arguments");
    r = FAILHARD;
    goto done;
  }

  if (fn) {
    ed.fin = openin(fn);
    if (!ed.fin) {
      r = FAILSOFT;
      goto done;
    }
  }
  else ed.fin = stdin;

  ed.curln = ed.lastln = 0;
  ed.line1 = ed.line2 = 0;
  ed.nlines = 0;
  strbuf_init(&ed.patbuf);
  strbuf_init(&ed.linebuf);

  bufinit(&ed);

  while (getline(&cmdbuf, '\n', ed.fin) > 0) {
    cmd = strbuf_ptr(&cmdbuf);
    i = 0;
    cursave = ed.curln;
    if ((status = getlist(&ed, cmd, &i)) == ED_OK) {
      /* TODO ck & do global command */
      status = docmd(&ed, cmd, &i, false);
    }
    if (status == ED_ERR) {
      message("?");
      ed.curln = MIN(cursave, ed.lastln);
    }
    else if (status == ED_END)
      break;
  }

  r = SUCCESS;

done:
  strbuf_free(&cmdbuf);
  strbuf_free(&ed.linebuf);
  strbuf_free(&ed.patbuf);
  buffree(&ed);
  if (ed.fin != stdin)
    fclose(ed.fin);
  return r;
}

static opstate /* get list of line numbers */
getlist(edstate *ped, const char *line, int *pi)
{
  int num;
  opstate status;

  ped->line2 = 0;
  ped->nlines = 0;
  status = getone(ped, line, pi, &num);
  while (status == ED_OK) {
    ped->line1 = ped->line2;
    ped->line2 = num;
    ped->nlines += 1;
    if (line[*pi] == ';')
      ped->curln = num;
    if (line[*pi] == ',' || line[*pi] == ';') {
      *pi += 1;
      status = getone(ped, line, pi, &num);
    }
    else
      break;
  }

  if (ped->nlines > 2)
    ped->nlines = 2;
  if (ped->nlines == 0)
    ped->line2 = ped->curln;
  if (ped->nlines <= 1)
    ped->line1 = ped->line2;
  if (status != ED_ERR)
    status = ED_OK;
  return status;
}

static opstate /* get one line number expr */
getone(edstate *ped, const char *line, int *pi, int *pnum)
{
  opstate status;
  int istart = *pi;

  *pnum = 0;
  status = getnum(ped, line, pi, pnum); /* 1st term */
  while (status == ED_OK) {
    skipblank(line, pi);
    if (line[*pi] == '+' || line[*pi] == '-') { /* additive terms */
      int nextnum;
      int sign = line[*pi] == '+' ? 1 : -1;
      *pi += 1; /* skip the operator */
      status = getnum(ped, line, pi, &nextnum);
      if (status == ED_OK)
        *pnum += sign * nextnum;
      else if (status == ED_END)
        status = ED_ERR;
    }
    else
      status = ED_END;
  }

  if (*pnum < 0 || *pnum > ped->lastln)
    status = ED_ERR;

  if (status == ED_ERR)
    return status;

  return *pi <= istart ? ED_END : ED_OK; /* TODO what if blanks before 1st term?? */
}

static opstate /* get a single line number term */
getnum(edstate *ped, const char *line, int *pi, int *pnum)
{
  skipblank(line, pi);

  if (isdigit(line[*pi])) {
    size_t n = scanint(line+*pi, pnum);
    *pi += n;
    return ED_OK;
  }

  if (line[*pi] == CURLINE) {
    *pnum = ped->curln;
    *pi += 1;
    return ED_OK;
  }

  if (line[*pi] == LASTLINE) {
    *pnum = ped->lastln;
    *pi += 1;
    return ED_OK;
  }

  if (line[*pi] == SCAN || line[*pi] == SCANBACK) {
    char direction = line[*pi];
    if (optpat(ped, line, pi) == ED_ERR)
      return ED_ERR;
    *pi += 1;
    return patscan(ped, direction, pnum);
  }

  return ED_END;
}

static opstate /* get optional search pattern */
optpat(edstate *ped, const char *line, int *pi)
{
  size_t n;

  if (line[*pi] == 0 || line[*pi+1] == 0) {
    strbuf_trunc(&ped->patbuf, 0);
    return ED_ERR;
  }

  if (line[*pi+1] == line[*pi]) {
    *pi += 1;
    return ED_OK;
  }

  n = makepat(line+*pi+1, line[*pi], &ped->patbuf);
  if (n == 0) {
    strbuf_trunc(&ped->patbuf, 0);
    return ED_ERR;
  }

  *pi += n + 1; /* pat and delim */
  return ED_OK;
}

static opstate /* next occurrence of pat after line num */
patscan(edstate *ped, char direction, int *pnum)
{
  int n = ped->curln;
  int flags = regex_none;
  const char *pat = strbuf_ptr(&ped->patbuf);
  const char *line;

  do {
    n = direction == SCAN ? nextln(ped, n) : prevln(ped, n);
    line = bufget(ped, n);
    if (match(line, pat, flags) >= 0) {
      *pnum = n;
      return ED_OK;
    }
  } while (n != ped->curln);

  return ED_ERR;
}

static opstate /* get right-hand-side of "s" command */
getsub(edstate *ped, const char *cmd, int *pi, bool *gflag)
{
  int n;
  char delim;
  if (!cmd[*pi] || !cmd[*pi+1])
    return ED_ERR;
  delim = cmd[*pi];
  *pi += 1;
  n = makesub(cmd+*pi, delim, &ped->subbuf);
  if (n < 0)
    return ED_ERR;
  *pi += n;
  *pi += 1;
  if (cmd[*pi] == 'g') {
    *pi += 1;
    *gflag = true;
  }
  else *gflag = false;
  return ED_OK;
}

static opstate /* set defaulted line numbers */
defaultlines(edstate *ped, int def1, int def2)
{
  if (ped->nlines == 0) {
    ped->line1 = def1;
    ped->line2 = def2;
  }
  return ped->line1 > 0 && ped->line1 <= ped->line2 ? ED_OK : ED_ERR;
}

static int /* the line after n, wrapping to 0 */
nextln(edstate *ped, int n)
{
  return n >= ped->lastln ? 0 : n + 1;
}

static int /* the line before n, wrapping to lastln */
prevln(edstate *ped, int n)
{
  return n <= 0 ? ped->lastln : n - 1;
}

static void /* init buffer (only line zero, scratch file) */
bufinit(edstate *ped)
{
  ped->buffer = 0;
  buf_push(ped->buffer, makebuf("\n")); /* line zero */
}

static void /* release resources (memory, scratch file, whatever) */
buffree(edstate *ped)
{
  size_t i, n;
  n = buf_size(ped->buffer);
  for (i = 1; i < n; i++)
    free((void *) ped->buffer[i].z);
  buf_free(ped->buffer);
}

static bufitem
makebuf(const char *z)
{
  bufitem item = { z, false };
  return item;
}

static void /* helper for bufmove */
reverse(bufitem *buf, int n1, int n2)
{
  while (n1 < n2) {
    bufitem temp = buf[n1];
    buf[n1] = buf[n2];
    buf[n2] = temp;
    n1++;
    n2--;
  }
}

static void /* move lines n1..n2 after n3 */
bufmove(bufitem *buf, int n1, int n2, int n3)
{
  if (n3 < n1 - 1) {
    reverse(buf, n3+1, n1-1);
    reverse(buf, n1, n2);
    reverse(buf, n3+1, n2);
  }
  else if (n3 > n2) {
    reverse(buf, n1, n2);
    reverse(buf, n2+1, n3);
    reverse(buf, n1, n3);
  }
}

static const char * /* get line n (immutable) */
bufget(edstate *ped, int n)
{
  if (n < 1 || n > ped->lastln)
    return ped->buffer[0].z; /* always "\n" */
  return ped->buffer[n].z;
}

static opstate /* add line after curln */
bufput(edstate *ped, const char *line)
{
  bufitem newitem;
  int m;
  const char *l = copyline(line);
  if (!l) return ED_ERR;
  /* append at end of buffer */
  newitem = makebuf(l);
  m = (int) buf_size(ped->buffer);
  buf_push(ped->buffer, newitem);
  /* move into place, i.e., to after current line */
  bufmove(ped->buffer, m, m, ped->curln);
  ped->curln += 1;
  ped->lastln += 1;
  return ED_OK;
}

static const char *
copyline(const char *z)
{
  size_t len;
  char *s;
  if (!z) return 0;
  len = strlen(z);
  s = malloc(len+1);
  if (!s) return 0;
  return memcpy(s, z, len+1);
}

static opstate
docmd(edstate *ped, const char *cmd, int *pi, bool glob)
{
  opstate status = ED_ERR;
  bool pflag = false;
  bool gflag = false;
  int line3;
  char cc = cmd[*pi];
  char nc = 0;

  if (cc) {
    *pi += 1;
    nc = cmd[*pi];
  }

  switch (cc) {
    case NEWLINE:
      if (ped->nlines == 0)
        ped->line2 = nextln(ped, ped->curln);
      status = doprint(ped, ped->line2, ped->line2);
      break;
    case PCMD:
      if (nc == NEWLINE)
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
          status = doprint(ped, ped->line1, ped->line2);
      break;
    case ACMD:
      if (nc == NEWLINE)
        status = doappend(ped, ped->line2, glob);
      break;
    case ICMD:
      if (nc == NEWLINE)
        status = ped->line2 == 0
          ? doappend(ped, 0, glob)
          : doappend(ped, prevln(ped, ped->line2), glob);
      break;
    case CCMD:
      if (nc == NEWLINE)
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
          if ((status = dodelete(ped, ped->line1, ped->line2)) == ED_OK)
            status = doappend(ped, prevln(ped, ped->line1), glob);
      break;
    case DCMD:
      if ((status = checkp(cmd, pi, &pflag)) == ED_OK) {
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK) {
          status = dodelete(ped, ped->line1, ped->line2);
          if (status == ED_OK && nextln(ped, ped->curln) != 0)
            ped->curln = nextln(ped, ped->curln);
        }
      }
      break;
    case MCMD:
      status = getone(ped, cmd, pi, &line3);
      if (status == ED_END) status = ED_ERR;
      if (status == ED_OK)
        if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
          if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
            status = domove(ped, line3);
      break;
    case JCMD:
      if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
        if ((status = defaultlines(ped, ped->curln, ped->curln+1)) == ED_OK)
          status = dojoin(ped, ped->line1, ped->line2);
      break;
    case SCMD:
      if ((status = optpat(ped, cmd, pi)) == ED_OK)
        if ((status = getsub(ped, cmd, pi, &gflag)) == ED_OK)
          if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
            if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
              status = dosubst(ped, gflag, glob);
      break;
    case EQCMD:
      if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
        printf("%d\n", ped->line2); /* line2 defaults to curln */
      break;
    case QCMD:
      if (nc == NEWLINE && ped->nlines == 0 && !glob)
        status = ED_END;
      break;
/* BEGIN DEBUG */
    case '?':
      debug("{nlines=%d, line1=%d, line2=%d, curln=%d, lastln=%d}\n",
        ped->nlines, ped->line1, ped->line2, ped->curln, ped->lastln);
      { size_t n = buf_size(ped->buffer); size_t i;
      debug("00: %s\n", ped->buffer[0].z);
      for (i = 1; i < n; i++)
        debug("%02zd: %s", i, ped->buffer[i].z); }
      status = ED_OK;
      break;
/* END DEBUG */
  }

  if (status == ED_OK && pflag)
    status = doprint(ped, ped->curln, ped->curln);
  return status;
}

static opstate
checkp(const char *cmd, int *pi, bool *ppflag)
{
  skipblank(cmd, pi);
  if (cmd[*pi] == PCMD) {
    *ppflag = true;
    *pi += 1;
    skipblank(cmd, pi);
  }
  else {
    *ppflag = false;
  }
  return cmd[*pi] == NEWLINE || cmd[*pi] == 0 ? ED_OK : ED_ERR;
}

static opstate /* print lines n1..n2 */
doprint(edstate *ped, int n1, int n2)
{
  int i;
  if (n1 <= 0) return ED_ERR;
  for (i = n1; i <= n2; i++) {
    const char *s = bufget(ped, i);
    putstr(s);
  }
  ped->curln = n2;
  return ED_OK;
}

static opstate /* append input lines after line n */
doappend(edstate *ped, int n, bool glob)
{
  if (glob)
    return ED_ERR;

  ped->curln = n;
  for (;;) {
    opstate status;
    const char *line;
    if (getline(&ped->linebuf, '\n', ped->fin) < 0)
      return ED_END;
    line = strbuf_ptr(&ped->linebuf);
    if (streq(line, ".\n"))
      return ED_OK;
    status = bufput(ped, line);
    if (status != ED_OK)
      return ED_ERR;
  }
}

static opstate /* delete lines n1..n2 */
dodelete(edstate *ped, int n1, int n2)
{
  if (n1 <= 0 || n1 > n2 || n2 > ped->lastln)
    return ED_ERR;
  /* move lines to end of buffer and "forget" them there */
  bufmove(ped->buffer, n1, n2, ped->lastln);
  ped->lastln -= n2 - n1 + 1;
  ped->curln = prevln(ped, n1);
  debug("{del %d..%d: leave with lastln=%d curln=%d}\n",
    n1, n2, ped->lastln, ped->curln);
  return ED_OK;
}

static opstate /* move line1..line2 to after line n3 */
domove(edstate *ped, int n3)
{
  if (ped->line1 <= 0 || (ped->line1 <= n3 && n3 <= ped->line2))
    return ED_ERR;
  bufmove(ped->buffer, ped->line1, ped->line2, n3);
  if (n3 > ped->line1) ped->curln = n3;
  else ped->curln = n3 + (ped->line2 - ped->line1 + 1);
  return ED_OK;
}

static opstate /* join lines n1..n2 */
dojoin(edstate *ped, int n1, int n2)
{
  int n, status;
  if (n1 <= 0 || n1 > n2 || n2 > ped->lastln)
    return ED_ERR;
  if (n1 == n2) {
    /* nothing to join, but set curln so xjp prints line x */
    ped->curln = n1;
    return ED_OK;
  }
  strbuf_trunc(&ped->linebuf, 0);
  for (n = n1; n <= n2; n++) {
    strbuf_addz(&ped->linebuf, bufget(ped, n));
    strbuf_trunc(&ped->linebuf, strbuf_len(&ped->linebuf) - 1);
  }
  strbuf_addc(&ped->linebuf, '\n');
  status = dodelete(ped, n1, n2);
  if (status != ED_OK) return status;
  return bufput(ped, strbuf_ptr(&ped->linebuf));
}

static opstate
dosubst(edstate *ped, bool gflag, bool glob)
{
  opstate status;
  strbuf newbuf = {0};
  int flags = regex_none;
  int n, subs;
  char *p;
  const char *pat = strbuf_ptr(&ped->patbuf);
  const char *sub = strbuf_ptr(&ped->subbuf);
  debug("{subst %s for %s, gflag=%d}\n", sub, pat, gflag);

  if (!gflag)
    flags |= regex_subjustone;

  status = glob ? ED_OK : ED_ERR;

  for (n = ped->line1; n <= ped->line2; n++) {
    const char *oldline = bufget(ped, n);
    strbuf_trunc(&newbuf, 0);
    subs = subline(oldline, pat, flags, sub, &newbuf);
    if (subs > 0) {
      const char *newline = strbuf_ptr(&newbuf);
      size_t len = strbuf_len(&newbuf);
      /* must not remove trailing newline (we have j for that) */
      if (len < 1 || newline[len-1] != '\n') {
        status = ED_ERR;
        break;
      }
      status = dodelete(ped, n, n);
      if (status != ED_OK) break;
      /* but newlines injected shall split the line */
      while (status == ED_OK && (p = strchr(newline, '\n'))) {
        char saved = p[1];
        p[1] = '\0';
        status = bufput(ped, newline);
        p[1] = saved;
        newline = p+1;
      }
      ped->line2 += ped->curln - n;
      n = ped->curln;
      if (status != ED_OK) break;
    }
  }

  strbuf_free(&newbuf);
  return status;
}

static void
message(const char *msg)
{
  puts(msg); /* appends a newline */
}

static void
debug(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

static int
parseopts(int argc, char **argv)
{
  int i, showhelp = 0;
  for (i = 1; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; /* no more option args */
    if (streq(p, "--")) { ++i; break; } /* end of option args */
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': showhelp = 1;
          break;
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
  fprintf(fp, "Usage: %s [file]\n", me);
  fprintf(fp, "Edit text files\n");
  fprintf(fp, "  -h   show this help text\n");
}
