
#include <setjmp.h>

#include "lines.h"
#include "common.h"

#define BUF_ABORT nomem()
#include "buf.h"

/* one line from fp, NUL terminate, return #chars (w/o NUL) */
size_t appendline(char **buf, FILE *fp)
{
  int c;
  size_t n0, n1;
  const int delim = '\n';

  n0 = buf_size(*buf);
  while ((c = getc(fp)) != EOF && c != delim) {
    buf_push(*buf, c);
  }
  if (c == delim) buf_push(*buf, delim);
  n1 = buf_size(*buf);

  /* fix incomplete last line */
  if (c == EOF && n1 > n0 && buf_peek(*buf) != delim) {
    buf_push(*buf, delim);
    n1 += 1;
  }

  buf_push(*buf, 0); /* terminate string */

  return ferror(fp) ? 0 : n1 - n0;
}

void truncline(char **buf)
{
  buf_clear(*buf);
}

void freeline(char **buf)
{
  buf_free(*buf);
}

/* the number of lines in plines */
size_t countlines(struct lines *plines)
{
  return buf_size(plines->linepos);
}

void clearlines(struct lines *plines)
{
  buf_clear(plines->linebuf);
  buf_clear(plines->linepos);
}

/* return <0 on error, 0 on eof */
int readlines(struct lines *plines, FILE *fp)
{
  size_t pos = 0;
  size_t limit = plines->chunksize;

  for (;;) {
    size_t n = appendline(&plines->linebuf, fp);
    if (n == 0) return 0; /* error or eof */
    buf_push(plines->linepos, pos);
    pos += n; /* advance position in linebuf */
    pos += 1; /* NUL is not counted by n */
    if (0 < limit && limit <= pos) return 1;
  }
}

/* write lines in linepos-order to fp */
void writelines(struct lines *plines, FILE *fp)
{
  const char *s;
  size_t i, k, n;
  n = buf_size(plines->linepos);
  for (i = 0; i < n; i++) {
    k = plines->linepos[i];
    s = plines->linebuf + k;
    fputs(s, fp);
  }
}

/* free the linebuf and linepos memory */
void freelines(struct lines *plines)
{
  buf_free(plines->linebuf);
  buf_free(plines->linepos);
}
