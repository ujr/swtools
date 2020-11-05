
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
#define ECMD     'e'    /* edit file */
#define FCMD     'f'    /* filename */
#define GCMD     'g'    /* global command */
#define ICMD     'i'    /* insert (before current line) */
#define JCMD     'j'    /* join lines */
#define MCMD     'm'    /* move line(s) */
#define PCMD     'p'    /* print */
#define QCMD     'q'    /* quit */
#define RCMD     'r'    /* read file */
#define SCMD     's'    /* substitute */
#define TCMD     't'    /* transliterate */
#define WCMD     'w'    /* write file */
#define XCMD     'x'    /* complement of global command */
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
  int line1;   /* first line number */
  int line2;   /* second line number */
  int nlines;  /* # of line numbers specified (0,1,2) */
  int curln;   /* num of current line (value of dot) */
  int lastln;  /* num of last line (value of $) */
  strbuf patbuf;   /* remembered search pattern */
  strbuf subbuf;   /* substitution text (ephemeral) */
  strbuf linebuf;  /* scratch buffer (ephemeral) */
  strbuf fnbuf;    /* remembered filename */
  bufitem *buffer; /* the line buffer (buf.h) */
} edstate;

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
static void bufinit(edstate *ped); /* init buffer (only line zero, scratch file) */
static void buffree(edstate *ped); /* release resources (memory, scratch file, whatever) */
static const char *bufget(edstate *ped, int n); /* get line n for editing */
static opstate bufput(edstate *ped, const char *line); /* add after curln */
static void bufmove(edstate *ped, int n1, int n2, int n3); /* move lines n1..n2 after n3 */
static bool getmark(edstate *ped, int n); /* get mark from line n */
static void putmark(edstate *ped, int n, bool mark); /* set mark on line n */
/* and related utilities */
static void reverse(bufitem *buf, int n1, int n2); /* helper for bufmove */
static bufitem makebuf(const char *z);
static const char *copyline(const char *z); /* malloc'ed copy */
static opstate updateln(edstate *ped, int n, const char *lines);

/* doing the commands */
static opstate docmd(edstate *ped, const char *cmd, int *pi, bool glob);
static opstate checkp(const char *cmd, int *pi, bool *ppflag);
static opstate doprint(edstate *ped, int n1, int n2);
static opstate doappend(edstate *ped, int n, bool glob);
static opstate dodelete(edstate *ped, int n1, int n2);
static opstate domove(edstate *ped, int n3);
static opstate dojoin(edstate *ped, int n1, int n2);
static opstate dosubst(edstate *ped, bool gflag, bool glob);
static opstate dotranslit(edstate *ped, bool allbut);
static opstate doread(edstate *ped, int n);
static opstate dowrite(edstate *ped, int n1, int n2);
static opstate ckglob(edstate *ped, const char *cmd, int *pi);
static opstate doglob(edstate *ped, const char *cmd, int *pi, int *pcursave);

static void message(const char *fmt, ...);

int
editcmd(int argc, char **argv)
{
  int r;
  strbuf cmdbuf = {0};
  edstate ed;
  opstate status;

  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (argc > 1 && argv[0] && argv[1]) {
    usage("too many arguments");
    r = FAILHARD;
    goto done;
  }

  ed.curln = ed.lastln = 0;
  ed.line1 = ed.line2 = 0;
  ed.nlines = 0;
  strbuf_init(&ed.linebuf);
  strbuf_init(&ed.patbuf);
  strbuf_init(&ed.subbuf);
  strbuf_init(&ed.fnbuf);

  bufinit(&ed);

  if (argc > 0 && *argv) {
    const char *fn = *argv;
    strbuf_addz(&ed.fnbuf, fn);
    if (doread(&ed, 0) != ED_OK)
      message("?");
  }

  while (getline(&cmdbuf, '\n', stdin) > 0) {
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
      message("?");
      ed.curln = MIN(cursave, ed.lastln);
    }
  }

  r = SUCCESS;

done:
  strbuf_free(&cmdbuf);
  strbuf_free(&ed.linebuf);
  strbuf_free(&ed.patbuf);
  strbuf_free(&ed.subbuf);
  strbuf_free(&ed.fnbuf);
  buffree(&ed);
  return r;
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

  if (cmd[*pi] == 0 || cmd[*pi+1] == 0) {
    strbuf_trunc(&ped->patbuf, 0);
    return ED_ERR;
  }

  if (cmd[*pi+1] == cmd[*pi]) {
    *pi += 1;
    return ED_OK;
  }

  n = makepat(cmd+*pi+1, cmd[*pi], &ped->patbuf);
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

static opstate /* scan /src/dst/c for translit */
gettrl(edstate *ped, const char *cmd, int *pi, bool *cflag)
{
  int i;
  char delim;
  if (!cmd[*pi] || !cmd[*pi+1]) return ED_ERR;
  delim = cmd[*pi]; *pi += 1;
  strbuf_trunc(&ped->linebuf, 0);
  i = dodash(cmd, *pi, delim, &ped->linebuf);
  if (cmd[i] != delim) return ED_ERR;
  *pi = i+1;
  if (!cmd[*pi]) return ED_ERR;
  strbuf_trunc(&ped->subbuf, 0);
  i = dodash(cmd, *pi, delim, &ped->subbuf);
  if (cmd[i] != delim) return ED_ERR;
  *pi = i+1;
  *cflag = cmd[*pi] == 'c'; /* complement */
  if (*cflag) *pi += 1;
  return ED_OK;
}

static opstate /* scan optional filename argument */
getfn(edstate *ped, const char *cmd, int *pi)
{
  int gotname = 0;
  int gotblank = cmd[*pi] == ' ';
  skipblank(cmd, pi);
  if (cmd[*pi] == '\n' || cmd[*pi] == 0)
    return strbuf_len(&ped->fnbuf) > 0 ? ED_OK : ED_ERR;
  if (!gotblank) return ED_ERR; /* one blank required */
  if (cmd[*pi] == '"') {
    strbuf_trunc(&ped->linebuf, 0);
    size_t n = scanstr(cmd + *pi, &ped->linebuf);
    if (!n) return ED_ERR;
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
    return ED_ERR; /* invalid trailing stuff */
  if (gotname) {
    strbuf_trunc(&ped->fnbuf, 0);
    strbuf_add(&ped->fnbuf, &ped->linebuf);
  }
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

static void /* initialize line buffer */
bufinit(edstate *ped)
{
  ped->buffer = 0;
  buf_push(ped->buffer, makebuf("\n")); /* line zero */
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

static bufitem
makebuf(const char *z)
{
  bufitem item = { z, false };
  return item;
}

static void /* move lines n1..n2 after n3 */
bufmove(edstate *ped, int n1, int n2, int n3)
{
  bufitem *buf = ped->buffer;
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
  bufmove(ped, m, m, ped->curln);
  ped->curln += 1;
  ped->lastln += 1;
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

static opstate /* replace line n with replacement line(s) */
updateln(edstate *ped, int n, const char *lines)
{
  opstate status;
  char *p;
  size_t len = strlen(lines);
  /* trailing newline is required */
  if (len < 1 || lines[len-1] != '\n')
    return ED_ERR;
  status = dodelete(ped, n, n);
  /* interior newlines will split the line */
  while (status == ED_OK && (p = strchr(lines, '\n'))) {
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
translit(const char *oldline, bool allbut, strbuf *src, strbuf *dst, strbuf *out)
{
  const char *s = strbuf_ptr(src);
  size_t slen = strbuf_len(src);
  const char *d = strbuf_ptr(dst);
  size_t dlen = strbuf_len(dst);
  bool drop = dlen <= 0; /* no dst: drop src matches */
  bool squash = slen > dlen || allbut; /* src longer: squash runs */
  int lastdst = dlen - 1; /* if squashing: use this char */
  char c;

  debug("{translit src=%s, dst=%s, cflag=%d}\n", s, d, allbut);

  if (drop) {
    while ((c = *oldline++) && c != '\n') {
      int j = xindex(allbut, s, slen, c, lastdst);
      if (j < 0) strbuf_addc(out, c); /* copy; else drop */
    }
  }
  else while ((c = *oldline++) && c != '\n') {
    int j = xindex(allbut, s, slen, c, lastdst);
    if (squash && j >= lastdst) {
      strbuf_addc(out, d[lastdst]); /* translate first char*/
      do j = xindex(allbut, s, slen, (c=*oldline++), lastdst);
      while (c && c != '\n' && j >= lastdst); /* and drop remaining */
    }
    if (c && c != '\n') {
      if (j >= 0) strbuf_addc(out, d[j]); /* translate */
      else strbuf_addc(out, c); /* no match: copy */
    }
  }
  strbuf_addc(out, '\n');
}

static opstate
docmd(edstate *ped, const char *cmd, int *pi, bool glob)
{
  opstate status = ED_ERR;
  bool pflag = false; /* print flag for many commands */
  bool gflag = false; /* global for substitute */
  bool cflag; /* complement for translit */
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
    case TCMD:
      if ((status = gettrl(ped, cmd, pi, &cflag)) == ED_OK)
        if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
          if ((status = defaultlines(ped, ped->curln, ped->curln)) == ED_OK)
            status = dotranslit(ped, cflag);
      break;
    case FCMD:
      if (ped->nlines == 0 && (status = getfn(ped, cmd, pi)) == ED_OK) {
        message("%s", strbuf_ptr(&ped->fnbuf));
        status = ED_OK;
      }
      break;
    case ECMD:
      if (ped->nlines == 0 && (status = getfn(ped, cmd, pi)) == ED_OK) {
        buffree(ped);
        bufinit(ped);
        status = doread(ped, 0);
      }
      break;
    case RCMD:
      if ((status = getfn(ped, cmd, pi)) == ED_OK)
        status = doread(ped, ped->line2);
      break;
    case WCMD:
      if ((status = getfn(ped, cmd, pi)) == ED_OK)
        if ((status = defaultlines(ped, 1, ped->lastln)) == ED_OK)
          status = dowrite(ped, ped->line1, ped->line2);
      break;
    case EQCMD:
      if ((status = checkp(cmd, pi, &pflag)) == ED_OK)
        message("%d", ped->line2); /* line2 defaults to curln */
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
      for (i = 0; i < n; i++)
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
    if (getline(&ped->linebuf, '\n', stdin) < 0)
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
  bufmove(ped, n1, n2, ped->lastln);
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
  bufmove(ped, ped->line1, ped->line2, n3);
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

static opstate /* substitute on line1..line2 */
dosubst(edstate *ped, bool gflag, bool glob)
{
  opstate status;
  strbuf newbuf = {0};
  int flags = regex_none;
  int n, subs;
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
      status = updateln(ped, n, newline);
      if (status != ED_OK) break;
    }
  }

  strbuf_free(&newbuf);
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

  for (n = ped->line1; n <= ped->line2; n++) {
    const char *oldline = bufget(ped, n);
    strbuf_trunc(&newbuf, 0);
    translit(oldline, allbut, src, dst, &newbuf);
    status = updateln(ped, n, strbuf_ptr(&newbuf));
    if (status != ED_OK) break;
  }

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
  if (!fp) return ED_ERR;

  ped->curln = n;
  while (getline(&ped->linebuf, '\n', fp) > 0) {
    numlines += 1;
    bufput(ped, strbuf_ptr(&ped->linebuf));
  }

  if (ferror(fp)) status = ED_ERR;
  if (fclose(fp)) status = ED_ERR;
  if (status == ED_OK)
    message("%d", numlines);
  return status;
}

static opstate /* write lines n1..n2 to file */
dowrite(edstate *ped, int n1, int n2)
{
  int n;
  opstate status = ED_OK;
  const char *fn = strbuf_ptr(&ped->fnbuf);

  FILE *fp = fopen(fn, "w");
  if (!fp) return ED_ERR;

  for (n = n1; n <= n2; n++) {
    const char *line = bufget(ped, n);
    fputs(line, fp);
  }

  if (ferror(fp)) status = ED_ERR;
  if (fclose(fp)) status = ED_ERR;
  if (status == ED_OK)
    message("%d", n2 - n1 + 1);
  return status;
}

static opstate /* check for global prefix, mark matching lines */
ckglob(edstate *ped, const char *cmd, int *pi)
{
  int n, gflag;
  const char *pat;
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
  pat = strbuf_ptr(&ped->patbuf);
  for (n = ped->line1; n <= ped->line2; n++) {
    const char *line = bufget(ped, n);
    int pos = match(line, pat, regex_none);
    putmark(ped, n, gflag ? pos >= 0 : pos < 0);
  }
  /* erase leftover marks */
  for (n = 1; n < ped->line1; n++)
    putmark(ped, n, false);
  for (n = ped->line2+1; n <= ped->lastln; n++)
    putmark(ped, n, false);
  return ED_OK;
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
  }

  return status;
}

static void
message(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  va_end(ap);
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
