/* edit */

#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "regex.h"
#include "strbuf.h"

static void memfail(void);
#define BUF_ABORT memfail()
#include "buf.h"

#define ACMD     'a'    /* append (after current line) */
#define CCMD     'c'    /* change (replace) line(s) */
#define DCMD     'd'    /* delete */
#define ECMD     'e'    /* edit file */
#define FCMD     'f'    /* filename */
#define GCMD     'g'    /* global command */
#define HCMD     'h'    /* help */
#define HHCMD    'H'    /* help mode toggle */
#define ICMD     'i'    /* insert (before current line) */
#define JCMD     'j'    /* join lines */
#define MCMD     'm'    /* move line(s) */
#define PCMD     'p'    /* print */
#define QCMD     'q'    /* quit */
#define QQCMD    'Q'    /* quit unconditionally */
#define RCMD     'r'    /* read file */
#define SCMD     's'    /* substitute */
#define TCMD     't'    /* transliterate */
#define UCMD     'u'    /* undo */
#define UUCMD    'U'    /* redo */
#define WCMD     'w'    /* write file */
#define WWCMD    'W'    /* append to file */
#define XCMD     'x'    /* complement of global command */
#define EQCMD    '='    /* show line number */
#define DBGCMD   '*'    /* show debug info to stderr */

#define NEWLINE  '\n'
#define CURLINE  '.'
#define LASTLINE '$'
#define SCAN     '/'
#define SCANBACK '?'

#define UNDOMOVE 'M'
#define UNDOEND  '$'

typedef struct {
  const char *z;
  bool mark;
} bufitem;

typedef struct {
  char cmd;        /* the undone command or 'M' */
  union {
    struct { int n1, n2, n3; } m;  /* if cmd is 'M' */
    struct { int curln, lastln; } l;   /* otherwise */
  } u;
} undoitem;

typedef struct {
  int line1;       /* first line number */
  int line2;       /* second line number */
  int nlines;      /* # of line numbers specified (0,1,2) */
  int curln;       /* num of current line (value of dot) */
  int lastln;      /* num of last line (value of $) */
  strbuf patbuf;   /* remembered search pattern */
  strbuf subbuf;   /* substitution text (ephemeral) */
  strbuf linebuf;  /* scratch buffer (ephemeral) */
  strbuf fnbuf;    /* remembered filename */
  bufitem *buffer; /* the line buffer (buf.h) */
  bool dirty;      /* set by edits, reset by full write */
  bool wantquit;   /* set by first q if buffer dirty */
  undoitem *ustk;  /* undo stack (buf.h) */
  size_t uptr;     /* pointer into undo stack */
  bool helpmode;   /* if true, automatically show errhelp */
  const char *errhelp;  /* explanation of most recent ? */
} edstate;

static void initialize(edstate *ped);
static void terminate(edstate *ped);

/* status of operations: returned by most functions */
typedef enum { ED_OK, ED_ERR, ED_END } opstate;

/* getting the line numbers and command arguments */
static opstate getlist(edstate *ped, const char *cmd, int *pi);
static opstate getone(edstate *ped, const char *cmd, int *pi, int *pnum);
static opstate getnum(edstate *ped, const char *cmd, int *pi, int *pnum);
static opstate optpat(edstate *ped, const char *cmd, int *pi);
static opstate patscan(edstate *ped, char direction, int *pnum);
static opstate getsub(edstate *ped, const char *cmd, int *pi, bool *gflag);
static opstate gettrl(edstate *ped, const char *cmd, int *pi, bool *cflag);
static opstate getfn(edstate *ped, const char *cmd, int *pi);
static opstate defaultlines(edstate *ped, int def1, int def2);
static int nextln(edstate *ped, int n);
static int prevln(edstate *ped, int n);

/* the line buffer interface */
static void bufinit(edstate *ped); /* init line buffer (make line zero) */
static void buffree(edstate *ped); /* release line buffer resources */
static const char *bufget(edstate *ped, int n); /* get line n for editing */
static opstate bufput(edstate *ped, const char *line); /* add after curln */
static void bufmove(edstate *ped, int n1, int n2, int n3); /* n1,n2 past n3 */
static bool getmark(edstate *ped, int n); /* get mark from line n */
static void putmark(edstate *ped, int n, bool mark); /* set mark on line n */
/* and lower-level utilities */
static void reverse(bufitem buf[], int n1, int n2);
static bool move(bufitem buf[], int n1, int n2, int n3);
static bool unmove(bufitem buf[], int n1, int n2, int n3);
static bufitem makebuf(const char *z);
static opstate append(edstate *ped, int n);
static opstate delete(edstate *ped, int n1, int n2);
static opstate joinlines(edstate *ped, int n1, int n2);
static opstate updateln(edstate *ped, int n, const char *lines);
static void translit(const char *in, bool allbut, strbuf *src, strbuf *dst, strbuf *out);

/* doing the commands */
static opstate docmd(edstate *ped, const char *cmd, int *pi, bool glob);
static opstate cknoargs(edstate *ped, char nc);
static opstate cknolines(edstate *ped);
static opstate checkp(edstate *ped, const char *cmd, int *pi, bool *ppflag);
static opstate doprint(edstate *ped, int n1, int n2);
static opstate doappend(edstate *ped, int n, bool glob);
static opstate dodelete(edstate *ped, int n1, int n2);
static opstate dochange(edstate *ped, int n1, int n2, bool glob);
static opstate domove(edstate *ped, int n3);
static opstate dojoin(edstate *ped, int n1, int n2);
static opstate dosubst(edstate *ped, bool gflag, bool glob);
static opstate dotranslit(edstate *ped, bool allbut);
static opstate doread(edstate *ped, int n);
static opstate dowrite(edstate *ped, int n1, int n2, bool append);
static opstate ckglob(edstate *ped, const char *cmd, int *pi);
static opstate doglob(edstate *ped, const char *cmd, int *pi, int *pcursave);
static opstate dodebug(edstate *ped, char subcmd);

/* undo/redo */
static void pushcmd(edstate *ped, char cmd);
static void pushmove(edstate *ped, int n1, int n2, int n3);
static void pushend(edstate *ped);
static void pushundo(edstate *ped, undoitem item);
static undoitem popundo(edstate *ped);
static opstate doundo(edstate *ped);
static opstate doredo(edstate *ped);

/* output routines and miscellaneous */
static void putline(const char *line); /* expects newline */
static void sigint(int signo);
static opstate checkint(opstate status);
static opstate ederr(edstate *ped, const char *msg);
static int parseopts(int argc, char **argv);
static void usage(const char *errmsg);

static volatile __sig_atomic_t intflag = 0;
static jmp_buf memjmp;

int
editcmd(int argc, char **argv)
{
  edstate ed;
  opstate status;
  strbuf cmdbuf = {0};
  int r;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  initialize(&ed);

  if (argc > 1 && argv[0] && argv[1]) {
    usage("too many arguments");
    r = FAILHARD;
    goto done;
  }

  signal(SIGINT, &sigint);
  signal(SIGTERM, SIG_IGN);

  bufinit(&ed);

  if (argc > 0 && *argv) {
    const char *fn = *argv;
    strbuf_addz(&ed.fnbuf, fn);
    if (doread(&ed, 0) != ED_OK)
      message("?");
  }

  strbuf_nomem(memfail);
  if (setjmp(memjmp)) {
    fputs("{jumped}\n", stderr);
    ed.errhelp = "out of memory";
    if (!ed.helpmode) message("?");
    else message("? %s", ed.errhelp);
  }

again:
  while (getline(&cmdbuf, NEWLINE, stdin) > 0) {
    const char *cmd = strbuf_ptr(&cmdbuf);
    int i = 0, cursave = ed.curln;
    if ((status = getlist(&ed, cmd, &i)) == ED_OK) {
      if ((status = ckglob(&ed, cmd, &i)) == ED_OK)
        status = doglob(&ed, cmd, &i, &cursave);
      else if (status != ED_ERR)
        status = docmd(&ed, cmd, &i, false);
    }
    if (status == ED_END) break;
    if (status == ED_ERR) {
      if (!ed.helpmode) message("?");
      else message("? %s", ed.errhelp ? ed.errhelp : "oops");
      ed.curln = MIN(cursave, ed.lastln);
    }
    else ed.errhelp = 0;
  }

  if (intflag) {
    message("?");
    intflag = 0;
    ed.errhelp = "interrupted";
    clearerr(stdin);
    clearerr(stdout);
    goto again;
  }

  r = SUCCESS;

done:
  strbuf_free(&cmdbuf);
  buffree(&ed);
  terminate(&ed);
  return r;
}

static void
initialize(edstate *ped)
{
  ped->curln = ped->lastln = 0;
  ped->line1 = ped->line2 = 0;
  ped->nlines = 0;
  strbuf_init(&ped->linebuf);
  strbuf_init(&ped->patbuf);
  strbuf_init(&ped->subbuf);
  strbuf_init(&ped->fnbuf);
  ped->buffer = 0;
  ped->dirty = false;
  ped->wantquit = false;
  ped->ustk = 0;
  ped->uptr = 0;
  ped->helpmode = false;
  ped->errhelp = 0;
}

static void
terminate(edstate *ped)
{
  strbuf_free(&ped->linebuf);
  strbuf_free(&ped->patbuf);
  strbuf_free(&ped->subbuf);
  strbuf_free(&ped->fnbuf);
  buf_free(ped->ustk);
  ped->ustk = 0;
}

static opstate /* get list of line numbers */
getlist(edstate *ped, const char *cmd, int *pi)
{
  int num;
  opstate status;

  ped->line2 = 0;
  ped->nlines = 0;
  status = getone(ped, cmd, pi, &num);
  while (status == ED_OK) {
    ped->line1 = ped->line2;
    ped->line2 = num;
    ped->nlines += 1;
    if (cmd[*pi] == ';')
      ped->curln = num;
    if (cmd[*pi] == ',' || cmd[*pi] == ';') {
      *pi += 1;
      status = getone(ped, cmd, pi, &num);
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
getone(edstate *ped, const char *cmd, int *pi, int *pnum)
{
  opstate status;
  int istart = *pi;

  *pnum = 0;
  status = getnum(ped, cmd, pi, pnum); /* 1st term */
  while (status == ED_OK) {
    skipblank(cmd, pi);
    if (cmd[*pi] == '+' || cmd[*pi] == '-') { /* additive terms */
      int nextnum;
      int sign = cmd[*pi] == '+' ? 1 : -1;
      *pi += 1; /* skip the operator */
      status = getnum(ped, cmd, pi, &nextnum);
      if (status == ED_OK)
        *pnum += sign * nextnum;
      else if (status == ED_END)
        status = ederr(ped, "syntax error");
    }
    else
      status = ED_END;
  }

  if (*pnum < 0 || *pnum > ped->lastln)
    status = ederr(ped, "line number out of range");

  if (status == ED_ERR)
    return status;

  return *pi <= istart ? ED_END : ED_OK;
}

static opstate /* get a single line number term */
getnum(edstate *ped, const char *cmd, int *pi, int *pnum)
{
  skipblank(cmd, pi);

  if (isdigit(cmd[*pi])) {
    size_t n = scanint(cmd+*pi, pnum);
    *pi += n;
    return ED_OK;
  }

  if (cmd[*pi] == CURLINE) {
    *pnum = ped->curln;
    *pi += 1;
    return ED_OK;
  }

  if (cmd[*pi] == LASTLINE) {
    *pnum = ped->lastln;
    *pi += 1;
    return ED_OK;
  }

  if (cmd[*pi] == SCAN || cmd[*pi] == SCANBACK) {
    char direction = cmd[*pi];
    if (optpat(ped, cmd, pi) == ED_ERR)
      return ED_ERR;
    *pi += 1;
    return patscan(ped, direction, pnum);
  }

  return ED_END;
}

static opstate /* get optional search pattern */
optpat(edstate *ped, const char *cmd, int *pi)
{
  size_t n;
  char delim;

  if (!cmd[*pi] || !cmd[*pi+1]) {
    strbuf_trunc(&ped->patbuf, 0);
    return ederr(ped, "missing search pattern");
  }

  if (cmd[*pi+1] == cmd[*pi]) {
    *pi += 1;
    return ED_OK; /* reuse previous pattern */
  }

  delim = cmd[*pi];
  n = makepat(cmd+*pi+1, delim, &ped->patbuf);
  if (n == 0) {
    strbuf_trunc(&ped->patbuf, 0);
    return ederr(ped, "invalid search pattern");
  }

  *pi += n + 1; /* skip pat and delim */
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

  return ederr(ped, "context search mismatch");
}

static opstate /* get right-hand-side of "s" command */
getsub(edstate *ped, const char *cmd, int *pi, bool *gflag)
{
  int n;
  char delim;
  if (!cmd[*pi] || !cmd[*pi+1])
    return ederr(ped, "expect replacement text");
  delim = cmd[*pi];
  *pi += 1;
  n = makesub(cmd+*pi, delim, &ped->subbuf);
  if (n < 0)
    return ederr(ped, "invalid replacement text");
  *pi += n;
  *pi += 1;
  if (cmd[*pi] == 'g') {
    *pi += 1;
    *gflag = true;
  }
  else *gflag = false;
  return ED_OK;
}

static opstate /* scan /src/dst/c for translit */
gettrl(edstate *ped, const char *cmd, int *pi, bool *cflag)
{
  int i;
  char delim;
  if (!cmd[*pi] || !cmd[*pi+1])
    return ederr(ped, "translit: missing arguments");
  delim = cmd[*pi]; *pi += 1;
  strbuf_trunc(&ped->linebuf, 0);
  i = dodash(cmd, *pi, delim, &ped->linebuf);
  if (cmd[i] != delim)
    return ederr(ped, "translit: syntax error");
  *pi = i+1;
  if (!cmd[*pi])
    return ederr(ped, "translit: missing arguments");
  strbuf_trunc(&ped->subbuf, 0);
  i = dodash(cmd, *pi, delim, &ped->subbuf);
  if (cmd[i] != delim)
    return ederr(ped, "translit: syntax error");
  *pi = i+1;
  *cflag = cmd[*pi] == 'c'; /* complement */
  if (*cflag) *pi += 1;
  return ED_OK;
}

static opstate /* scan optional filename argument */
getfn(edstate *ped, const char *cmd, int *pi)
{
  size_t n;
  int gotname = 0;
  int gotblank = cmd[*pi] == ' ';
  skipblank(cmd, pi);
  if (cmd[*pi] == NEWLINE || !cmd[*pi])
    return strbuf_len(&ped->fnbuf) > 0 ? ED_OK
      : ederr(ped, "no remembered filename");
  if (!gotblank) /* at least one blank required */
    return ederr(ped, "syntax error");
  if (cmd[*pi] == '"') {
    strbuf_trunc(&ped->linebuf, 0);
    if (!(n = scanstr(cmd + *pi, &ped->linebuf)))
      return ederr(ped, "filename: bad string argument");
    *pi += n;
    gotname = 1;
  }
  else {
    int i = *pi;
    while (isgraph(cmd[i])) i += 1;
    gotname = i > *pi;
    strbuf_trunc(&ped->linebuf, 0);
    strbuf_addb(&ped->linebuf, cmd + *pi, i - *pi);
    *pi = i;
  }
  skipblank(cmd, pi);
  if (cmd[*pi] != 0 && cmd[*pi] != NEWLINE)
    return ederr(ped, "filename: too many arguments");
  if (gotname) { /* replace remembered name */
    strbuf_trunc(&ped->fnbuf, 0);
    strbuf_add(&ped->fnbuf, &ped->linebuf);
  }
  /* else: keep remembered name */
  return ED_OK;
}

static opstate /* set defaulted line numbers */
defaultlines(edstate *ped, int def1, int def2)
{
  if (ped->nlines == 0) {
    ped->line1 = def1;
    ped->line2 = def2;
  }
  if (ped->line1 > 0 && ped->line1 <= ped->line2)
    return ED_OK;
  return ederr(ped, "line numbers out of range");
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

static void /* initialize line buffer */
bufinit(edstate *ped)
{
  bufitem line0 = makebuf("\n");
  ped->buffer = 0; /* required by buf.h */
  buf_push(ped->buffer, line0);
  ped->curln = ped->lastln = 0;
  ped->dirty = false;
}

static void /* release buffer resources */
buffree(edstate *ped)
{
  size_t i, n;
  n = buf_size(ped->buffer);
  for (i = 1; i < n; i++)
    free((void *) ped->buffer[i].z);
  buf_free(ped->buffer);
}

static void /* move lines n1..n2 after n3 */
bufmove(edstate *ped, int n1, int n2, int n3)
{
  if (move(ped->buffer, n1, n2, n3)) {
    ped->dirty = true;
    pushmove(ped, n1, n2, n3);
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
  const char *copy = strclone(line);
  if (!copy) return ederr(ped, "out of memory");
  /* append at end of buffer */
  newitem = makebuf(copy);
  m = (int) buf_size(ped->buffer);
  buf_push(ped->buffer, newitem);
  /* move into place, i.e., to after current line */
  bufmove(ped, m, m, ped->curln);
  ped->curln += 1;
  ped->lastln += 1;
  ped->dirty = true;
  return ED_OK;
}

static bool /* get mark from line n */
getmark(edstate *ped, int n)
{
  return ped->buffer[n].mark;
}

static void /* set mark on line n */
putmark(edstate *ped, int n, bool mark)
{
  ped->buffer[n].mark = mark;
}

static bufitem
makebuf(const char *z)
{
  bufitem item = { z, false };
  return item;
}

static void /* reverse lines n1..n2 in buffer */
reverse(bufitem buf[], int n1, int n2)
{
  while (n1 < n2) {
    bufitem temp = buf[n1];
    buf[n1] = buf[n2];
    buf[n2] = temp;
    n1++;
    n2--;
  }
}

static bool /* move n1..n2 to after n3 */
move(bufitem buf[], int n1, int n2, int n3)
{
  if (n3 < n1 - 1) {
    reverse(buf, n3+1, n1-1);
    reverse(buf, n1, n2);
    reverse(buf, n3+1, n2);
    return true;
  }
  if (n3 > n2) {
    reverse(buf, n1, n2);
    reverse(buf, n2+1, n3);
    reverse(buf, n1, n3);
    return true;
  }
  return false; /* no move: target in n1..n2 */
}

static bool /* the inverse of move(n1,n2,n3) */
unmove(bufitem buf[], int n1, int n2, int n3)
{
  if (n3 < n1 - 1) {
    reverse(buf, n3+1, n2);
    reverse(buf, n1, n2);
    reverse(buf, n3+1, n1-1);
    return true;
  }
  if (n3 > n2) {
    reverse(buf, n1, n3);
    reverse(buf, n2+1, n3);
    reverse(buf, n1, n2);
    return true;
  }
  return false;
}

static opstate /* read and append lines after line n */
append(edstate *ped, int n)
{
  const char *line;
  opstate status = ED_OK;
  ped->curln = n;
  for (;;) {
    if (getline(&ped->linebuf, NEWLINE, stdin) <= 0)
      return ED_END;
    line = strbuf_ptr(&ped->linebuf);
    if (streq(line, ".\n")) return ED_OK;
    status = bufput(ped, line);
    if (status != ED_OK) break;
    if ((status = checkint(status)) != ED_OK) break;
  }
  return status;
}

static opstate /* delete lines n1..n2 */
delete(edstate *ped, int n1, int n2)
{
  /* move lines to end of buffer and "forget" them there */
  bufmove(ped, n1, n2, ped->lastln);
  ped->lastln -= n2 - n1 + 1;
  ped->curln = prevln(ped, n1);
  return ED_OK;
}

static opstate /* join lines n1..n2 into linebuf */
joinlines(edstate *ped, int n1, int n2)
{
  int n;
  opstate status = ED_OK;
  strbuf_trunc(&ped->linebuf, 0);
  for (n = n1; n <= n2; n++) {
    /* append lines n1..n2 but not the trailing newlines */
    strbuf_addz(&ped->linebuf, bufget(ped, n));
    strbuf_trunc(&ped->linebuf, strbuf_len(&ped->linebuf) - 1);
    if ((status = checkint(status)) != ED_OK) return status;
  }
  strbuf_addc(&ped->linebuf, NEWLINE);
  return status;
}

static opstate /* replace line n with line(s), update curln, lastln, line2 */
updateln(edstate *ped, int n, const char *lines)
{
  opstate status;
  char *p;
  size_t len = strlen(lines);
  /* trailing newline is required */
  if (len < 1 || lines[len-1] != NEWLINE)
    return ederr(ped, "must not remove trailing newline");
  if (streq(lines, bufget(ped, n)))
    return ED_OK; /* line did not change */
  status = delete(ped, n, n);
  /* interior newlines will split the line */
  while (status == ED_OK && (p = strchr(lines, NEWLINE))) {
    char saved = p[1];
    p[1] = '\0';
    status = bufput(ped, lines);
    p[1] = saved;
    lines = p+1;
  }
  ped->line2 += ped->curln - n;
  n = ped->curln;
  return status;
}

static void
translit(const char *in, bool allbut, strbuf *src, strbuf *dst, strbuf *out)
{
  const char *s = strbuf_ptr(src);
  size_t slen = strbuf_len(src);
  const char *d = strbuf_ptr(dst);
  size_t dlen = strbuf_len(dst);
  bool drop = dlen <= 0; /* no dst: drop src matches */
  bool squash = slen > dlen || allbut; /* src longer: squash runs */
  int lastdst = dlen - 1; /* if squashing: use this char */
  char c;

  if (drop) {
    while ((c = *in++)) {
      int j = xindex(allbut, s, slen, c, lastdst);
      if (j < 0) strbuf_addc(out, c); /* copy; else drop */
    }
  }
  else while ((c = *in++)) {
    int j = xindex(allbut, s, slen, c, lastdst);
    if (squash && j >= lastdst) {
      strbuf_addc(out, d[lastdst]); /* translate first char*/
      do j = xindex(allbut, s, slen, (c=*in++), lastdst);
      while (c && j >= lastdst); /* and drop remaining */
    }
    if (c) {
      if (j >= 0) strbuf_addc(out, d[j]); /* translate */
      else strbuf_addc(out, c); /* no match: copy */
    }
  }
}

static opstate
docmd(edstate *ped, const char *cmd, int *pi, bool glob)
{
  opstate status = ED_ERR;
  bool pflag = false;  /* print flag for many commands */
  bool gflag = false;  /* global for substitute */
  bool cflag;          /* complement for translit */
  char cc = cmd[*pi];  /* current cmd character */
  char nc = 0;         /* next cmd char, 0 if none */
  int line3;

  if (cc) {
    *pi += 1;
    nc = cmd[*pi];
  }

  switch (cc) {
    case NEWLINE: /* print next line */
      if (ped->nlines == 0)
        ped->line2 = nextln(ped, ped->curln);
      status = doprint(ped, ped->line2, ped->line2);
      break;
    case PCMD: /* print */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
          status = doprint(ped, ped->line1, ped->line2);
      break;
    case ACMD: /* append */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        status = doappend(ped, ped->line2, glob);
      break;
    case ICMD: /* insert */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        status = ped->line2 == 0
          ? doappend(ped, 0, glob)
          : doappend(ped, prevln(ped, ped->line2), glob);
      break;
    case CCMD: /* change */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
          status = dochange(ped, ped->line1, ped->line2, glob);
      break;
    case DCMD: /* delete */
      if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
        if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK) {
          status = dodelete(ped, ped->line1, ped->line2);
          if (status == ED_OK && nextln(ped, ped->curln) != 0)
            ped->curln = nextln(ped, ped->curln);
        }
      break;
    case MCMD: /* move */
      status = getone(ped, cmd, pi, &line3);
      if (status == ED_END)
        status = ederr(ped, "expect line address argument");
      else if (status == ED_OK)
        if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
          if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
            status = domove(ped, line3);
      break;
    case JCMD: /* join */
      if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
        if ((status = defaultlines(ped, ped->curln, ped->curln+1)) == ED_OK)
          status = dojoin(ped, ped->line1, ped->line2);
      break;
    case SCMD: /* substitute */
      if ((status = optpat(ped, cmd, pi)) == ED_OK)
        if ((status = getsub(ped, cmd, pi, &gflag)) == ED_OK)
          if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
            if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
              status = dosubst(ped, gflag, glob);
      break;
    case TCMD: /* translit */
      if ((status = gettrl(ped, cmd, pi, &cflag)) == ED_OK)
        if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
          if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
            status = dotranslit(ped, cflag);
      break;
    case FCMD: /* filename */
      if ((status = cknolines(ped)) == ED_OK)
        if ((status = getfn(ped, cmd, pi)) == ED_OK) {
          message("%s", strbuf_ptr(&ped->fnbuf));
          status = ED_OK;
        }
      break;
    case ECMD: /* edit */
      if ((status = cknolines(ped)) == ED_OK)
        if ((status = getfn(ped, cmd, pi)) == ED_OK) {
          buffree(ped);
          bufinit(ped);
          status = doread(ped, 0);
          ped->dirty = false;
        }
      break;
    case RCMD: /* read */
      if ((status = getfn(ped, cmd, pi)) == ED_OK)
        status = doread(ped, ped->line2);
      break;
    case WCMD: /* write */
    case WWCMD: /* append */
      if ((status = getfn(ped, cmd, pi)) == ED_OK)
        if ((status = defaultlines(ped, 1, ped->lastln)) == ED_OK)
          status = dowrite(ped, ped->line1, ped->line2, cc == WWCMD);
      break;
    case EQCMD: /* show line number */
      if ((status = checkp(ped, cmd, pi, &pflag)) == ED_OK)
        message("%d", ped->line2); /* line2 defaults to curln */
      break;
    case HCMD: /* help */
      message("! %s", ped->errhelp ? ped->errhelp : "alright");
      status = ED_OK;
      break;
    case HHCMD: /* help mode */
      ped->helpmode = !ped->helpmode;
      status = ED_OK;
      break;
    case QCMD: /* quit with prompt */
    case QQCMD: /* quit unconditionally */
      if (nc == NEWLINE && ped->nlines == 0 && !glob) {
        if (!ped->dirty || ped->wantquit || cc == QQCMD)
          status = ED_END;
        else {
          ped->wantquit = true;
          status = ederr(ped, "again to quit");
        }
      }
      break;
    case UCMD: /* undo */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        if ((status = cknolines(ped)) == ED_OK)
          status = doundo(ped);
      break;
    case UUCMD: /* redo */
      if ((status = cknoargs(ped, nc)) == ED_OK)
        if ((status = cknolines(ped)) == ED_OK)
          status = doredo(ped);
      break;
    case DBGCMD: /* debug info */
      status = dodebug(ped, nc);
      break;
    default:
      status = ederr(ped, "unknown command");
      break;
  }

  if (cc != QCMD && cc != HCMD)
    ped->wantquit = false;
  if (status == ED_OK && pflag)
    status = doprint(ped, ped->curln, ped->curln);
  return status;
}

static opstate
cknoargs(edstate *ped, char nc)
{
  if (!nc || nc == NEWLINE) return ED_OK;
  return ederr(ped, "expect no arguments");
}

static opstate
cknolines(edstate *ped)
{
  if (ped->nlines == 0) return ED_OK;
  return ederr(ped, "expect no line addresses");
}

static opstate
checkp(edstate *ped, const char *cmd, int *pi, bool *ppflag)
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
  if (cmd[*pi] == NEWLINE || !cmd[*pi]) return ED_OK;
  return ederr(ped, "too many arguments");
}

static opstate /* print lines n1..n2 */
doprint(edstate *ped, int n1, int n2)
{
  int n;
  opstate status = ED_OK;
  if (n1 <= 0 || n1 > n2 || n2 > ped->lastln)
    return ederr(ped, "line numbers out of range");
  for (n = n1; n <= n2; n++) {
    const char *s = bufget(ped, n);
    putline(s);
    if ((status = checkint(status)) != ED_OK) break;
  }
  ped->curln = n2;
  return status;
}

static opstate /* append input lines after line n */
doappend(edstate *ped, int n, bool glob)
{
  opstate status;
  if (glob)
    return ederr(ped, "not implemented with global command");
  pushcmd(ped, 'a');
  status = append(ped, n);
  pushend(ped);
  return status;
}

static opstate /* delete lines n1..n2 */
dodelete(edstate *ped, int n1, int n2)
{
  opstate status;
  int last = ped->lastln;
  if (n1 <= 0 || n1 > n2 || n2 > last)
    return ederr(ped, "line numbers out of range");
  pushcmd(ped, 'd');
  status = delete(ped, n1, n2);
  pushend(ped);
  return status;
}

static opstate /* replace lines n1..n2 by user input */
dochange(edstate *ped, int n1, int n2, bool glob)
{
  opstate status;
  if (glob)
    return ederr(ped, "not implemented with global command");
  pushcmd(ped, 'c');
  if ((status = delete(ped, n1, n2)) == ED_OK)
    status = append(ped, prevln(ped, n1));
  pushend(ped);
  return status;
}

static opstate /* move line1..line2 to after line n3 */
domove(edstate *ped, int n3)
{
  int n1 = ped->line1;
  int n2 = ped->line2;
  int k = n2 - n1 + 1;
  if (n1 <= 0 || n1 > n2 || n2 > ped->lastln)
    return ederr(ped, "line numbers out of range");
  if (n1 <= n3 && n3 <= n2)
    return ederr(ped, "target address in source range");
  pushcmd(ped, 'm');
  bufmove(ped, n1, n2, n3);
  if (n3 > n1) ped->curln = n3;
  else ped->curln = n3 + k;
  pushend(ped);
  return ED_OK;
}

static opstate /* join lines n1..n2 */
dojoin(edstate *ped, int n1, int n2)
{
  opstate status;
  if (n1 <= 0 || n1 > n2 || n2 > ped->lastln)
    return ederr(ped, "line numbers out of range");
  if (n1 == n2)
    return ederr(ped, "cannot join one line");
  status = joinlines(ped, n1, n2);
  if (status != ED_OK) return status;
  pushcmd(ped, 'j');
  if ((status = delete(ped, n1, n2)) == ED_OK)
    status = bufput(ped, strbuf_ptr(&ped->linebuf));
  pushend(ped);
  return status;
}

static opstate /* substitute on line1..line2 */
dosubst(edstate *ped, bool gflag, bool glob)
{
  opstate status;
  int flags = regex_none;
  int n, subs, tally = 0;
  const char *pat = strbuf_ptr(&ped->patbuf);
  const char *sub = strbuf_ptr(&ped->subbuf);
  debug("{subst %s for %s, gflag=%d}", sub, pat, gflag);

  if (!gflag)
    flags |= regex_subjustone;

  status = ED_OK;

  pushcmd(ped, 's');

  for (n = ped->line1; n <= ped->line2; n++) {
    const char *oldline = bufget(ped, n);
    strbuf_trunc(&ped->linebuf, 0);
    subs = subline(oldline, pat, flags, sub, &ped->linebuf);
    if (subs > 0) {
      const char *newline = strbuf_ptr(&ped->linebuf);
      status = updateln(ped, n, newline);
      if (status != ED_OK) break;
      n = ped->curln; /* adjust in case line was split */
      tally += subs;
    }
    if ((status = checkint(status)) != ED_OK) break;
  }

  if (status == ED_OK && tally == 0 && !glob) {
    popundo(ped);
    status = ederr(ped, "no substitutions made");
  }
  else pushend(ped);

  return status;
}

static opstate /* transliterate on lines line1..line2 */
dotranslit(edstate *ped, bool allbut)
{
  int n;
  strbuf *src = &ped->linebuf;
  strbuf *dst = &ped->subbuf;
  strbuf newbuf = {0};
  opstate status = ED_OK;

  pushcmd(ped, 't');
  for (n = ped->line1; n <= ped->line2; n++) {
    const char *oldline = bufget(ped, n);
    strbuf_trunc(&newbuf, 0);
    translit(oldline, allbut, src, dst, &newbuf);
    status = updateln(ped, n, strbuf_ptr(&newbuf));
    if ((status = checkint(status)) != ED_OK) break;
    n = ped->curln; /* adjust in case line was split */
  }
  pushend(ped);
  strbuf_free(&newbuf);
  return status;
}

static opstate /* read file, insert after line n */
doread(edstate *ped, int n)
{
  int numlines = 0;
  opstate status = ED_OK;
  const char *fn = strbuf_ptr(&ped->fnbuf);

  FILE *fp = fopen(fn, "r");
  if (!fp) return ederr(ped, "cannot open file");

  pushcmd(ped, 'r');

  ped->curln = n;
  while (getline(&ped->linebuf, NEWLINE, fp) > 0) {
    numlines += 1;
    bufput(ped, strbuf_ptr(&ped->linebuf));
    if ((status = checkint(status)) != ED_OK) break;
  }

  pushend(ped);

  if (ferror(fp)) status = ederr(ped, "I/O error");
  if (fclose(fp)) status = ederr(ped, "I/O error");
  if (status == ED_OK)
    message("%d", numlines);
  return status;
}

static opstate /* write lines n1..n2 to file */
dowrite(edstate *ped, int n1, int n2, bool append)
{
  int n;
  opstate status = ED_OK;
  const char *fn = strbuf_ptr(&ped->fnbuf);

  FILE *fp = fopen(fn, append ? "a" : "w");
  if (!fp) return ederr(ped, "cannot open file");

  for (n = n1; n <= n2; n++) {
    const char *line = bufget(ped, n);
    if (fputs(line, fp) < 0) break;
  }

  if (ferror(fp)) status = ederr(ped, "I/O error");
  if (fclose(fp)) status = ederr(ped, "I/O error");
  if (status == ED_OK) {
    message("%d", n2 - n1 + 1);
    if (n1 == 1 && n2 == ped->lastln)
      ped->dirty = false; /* all saved */
  }
  return status;
}

static opstate /* check for global prefix, mark matching lines */
ckglob(edstate *ped, const char *cmd, int *pi)
{
  int n, gflag;
  const char *pat;
  opstate status;
  switch (cmd[*pi]) {
    case GCMD: gflag = 1; break;
    case XCMD: gflag = 0; break;
    default: return ED_END;
  }
  *pi += 1;
  if (optpat(ped, cmd, pi) != ED_OK)
    return ED_ERR;
  if (defaultlines(ped, 1, ped->lastln) != ED_OK)
    return ED_ERR;
  *pi += 1;
  status = ED_OK;
  pat = strbuf_ptr(&ped->patbuf);
  for (n = ped->line1; n <= ped->line2; n++) {
    const char *line = bufget(ped, n);
    int pos = match(line, pat, regex_none);
    putmark(ped, n, gflag ? pos >= 0 : pos < 0);
    if ((status = checkint(status)) != ED_OK) break;
  }
  /* erase leftover marks */
  for (n = 1; n < ped->line1; n++)
    putmark(ped, n, false);
  for (n = ped->line2+1; n <= ped->lastln; n++)
    putmark(ped, n, false);
  return status;
}

static opstate /* do cmd[i..] on all marked lines */
doglob(edstate *ped, const char *cmd, int *pi, int *pcursave)
{
  int count = 0;
  int istart = *pi;
  int n = ped->line1;
  opstate status = ED_OK;

  /* be sure to treat all marked lines, regardless of how
     much the individual commands rearrange the lines; done
     when no marked lines remain after last successful cmd */
  while (count <= ped->lastln && status == ED_OK)
  {
    if (getmark(ped, n)) {
      putmark(ped, n, false);
      ped->curln = n;
      *pcursave = n;
      *pi = istart;
      if ((status = getlist(ped, cmd, pi)) == ED_OK)
        if ((status = docmd(ped, cmd, pi, true)) == ED_OK)
          count = 0;
    }
    else {
      n = nextln(ped, n);
      count += 1;
    }
    if ((status = checkint(status)) != ED_OK) break;
  }

  return status;
}

static opstate
dodebug(edstate *ped, char subcmd)
{
  size_t i;
  size_t bufsize = buf_size(ped->buffer);
  if (subcmd == DBGCMD) {
    for (i = 0; i < bufsize && !intflag; i++)
      fprintf(stderr, "%02zd: %s", i, ped->buffer[i].z);
  }
  else if (subcmd == UCMD) {
    fprintf(stderr, "Undo stack:");
    for (i = 0; i < buf_size(ped->ustk); i++) {
      undoitem *pitem = &ped->ustk[i];
      if (pitem->cmd == UNDOEND)
        fprintf(stderr, " (end cur=%d last=%d)", pitem->u.l.curln, pitem->u.l.lastln);
      else if (pitem->cmd == UNDOMOVE)
        fprintf(stderr, " (move %d %d %d)", pitem->u.m.n1, pitem->u.m.n2, pitem->u.m.n3);
      else
        fprintf(stderr, " (cmd %c cur=%d last=%d)", pitem->cmd, pitem->u.l.curln, pitem->u.l.lastln);
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "{command: nlines=%d line1=%d line2=%d wantquit=%d}\n",
    ped->nlines, ped->line1, ped->line2, ped->wantquit);
  fprintf(stderr, "{buffer: curln=%d lastln=%d bufsize=%zd dirty=%d}\n",
    ped->curln, ped->lastln, bufsize, ped->dirty);
  fprintf(stderr, "{ustack: ptr=%zd size=%zd (NB: u is --ptr, U is ptr++)}\n",
    ped->uptr, buf_size(ped->ustk));
  return ED_OK;
}

static void
pushundo(edstate *ped, undoitem item)
{
  size_t n;
  /* drop excess undo items (can redo only after undo) */
  for (n=buf_size(ped->ustk); n > ped->uptr; n=buf_size(ped->ustk))
    (void) buf_pop(ped->ustk);
  buf_push(ped->ustk, item);
  ped->uptr++;
}

static void
pushcmd(edstate *ped, char cmd)
{
  undoitem item;
  item.cmd = cmd;
  item.u.l.curln = ped->curln;
  item.u.l.lastln = ped->lastln;
  pushundo(ped, item);
}

static void
pushmove(edstate *ped, int n1, int n2, int n3)
{
  undoitem item;
  item.cmd = UNDOMOVE;
  item.u.m.n1 = n1;
  item.u.m.n2 = n2;
  item.u.m.n3 = n3;
  pushundo(ped, item);
}

static void
pushend(edstate *ped)
{
  undoitem item;
  item.cmd = UNDOEND;
  item.u.l.curln = ped->curln;
  item.u.l.lastln = ped->lastln;
  pushundo(ped, item);
}

static undoitem
popundo(edstate *ped)
{
  static undoitem dummy = {'?',{{0,0,0}}};
  return ped->uptr > 0 ? ped->ustk[--ped->uptr] : dummy;
}

static opstate /* try to undo the last command */
doundo(edstate *ped)
{
  if (ped->uptr == 0)
    return ederr(ped, "undo stack empty");

  /* undo stack: ... cmd { move } [ end ] ^ ... */

  while (ped->uptr > 0) {
    undoitem item = ped->ustk[--ped->uptr];
    if (item.cmd == UNDOEND) continue;
    if (item.cmd == UNDOMOVE) {
      unmove(ped->buffer, item.u.m.n1, item.u.m.n2, item.u.m.n3);
    }
    else {
      ped->curln = item.u.l.curln;
      ped->lastln = item.u.l.lastln;
      break;
    }
  }
  return ED_OK;
}

static opstate /* redo last undone command */
doredo(edstate *ped)
{
  size_t n = buf_size(ped->ustk);
  if (ped->uptr >= n)
    return ederr(ped, "no undone actions to redo");

  /* undo stack: ... ^ cmd { move } [ end ] ... */

  while (ped->uptr < n) {
    undoitem item = ped->ustk[ped->uptr++];
    if (item.cmd == UNDOEND) {
      ped->curln = item.u.l.curln;
      ped->lastln = item.u.l.lastln;
      break;
    }
    if (item.cmd == UNDOMOVE) {
      move(ped->buffer, item.u.m.n1, item.u.m.n2, item.u.m.n3);
    }
    else continue;
  }
  return ED_OK;
}

static void
putline(const char *line)
{
  fputs(line, stdout);
}

static void
sigint(int signo)
{
  UNUSED(signo);
  intflag = 1;
  debug("{SIGINT}");
  signal(SIGINT, &sigint); /* re-establish handler */
}

static opstate /* map OK to END if interrupted */
checkint(opstate status)
{
  return intflag && status == ED_OK ? ED_END : status;
}

static opstate /* set message and return ERR */
ederr(edstate *ped, const char *msg)
{
  ped->errhelp = msg;
  return ED_ERR;
}

static void
memfail(void)
{
  debug("{MEMFAIL}");
  longjmp(memjmp, 1);
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
