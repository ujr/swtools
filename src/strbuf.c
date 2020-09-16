/* strbuf.c - growable char buffer */

#include <assert.h>  /* assert() */
#include <stdarg.h>  /* va_list etc. */
#include <stddef.h>  /* size_t */
#include <stdio.h>   /* vsnprintf() */
#include <stdlib.h>  /* malloc(), realloc(), free() */
#include <string.h>  /* memcpy(), strlen() */

#include "strbuf.h"

#define GROWFUNC(x) (((x)+16)*3/2) /* lifted from Git */
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define LEN(sp)     ((sp)->len)
#define SIZE(sp)    ((sp)->size & ~1)           /* mask off lsb */
#define HASROOM(sp,n) (LEN(sp)+(n)+1 <= SIZE(sp))  /* +1 for \0 */
#define NEXTSIZE(sp)  GROWFUNC(SIZE(sp))   /* next default size */
#define SETFAILED(sp) ((sp)->size |= 1)      /* set lsb to flag */

int /* append the string buffer sq */
strbuf_add(strbuf *sp, strbuf *sq)
{
  assert(sq != 0);
  return strbuf_addb(sp, sq->buf, sq->len);
}

int /* append the single character c */
strbuf_addc(strbuf *sp, int c)
{
  assert(sp != 0);
  if (!sp->buf || !HASROOM(sp, 1)) {
    if (!strbuf_ready(sp, 1)) return 0; /* nomem */
  }
  sp->buf[sp->len++] = (unsigned char) c;
  sp->buf[sp->len] = '\0';
  return 1;
}

int /* append the \0 terminated string z */
strbuf_addz(strbuf *sp, const char *z)
{
  return strbuf_addb(sp, z, z ? strlen(z) : 0);
}

int /* append buf[0..len-1] to sp */
strbuf_addb(strbuf *sp, const char *buf, size_t len)
{
  assert(sp != 0);
  if (!buf) len = 0;
  if (!strbuf_ready(sp, len)) return 0; /* nomem */
  if (buf) memcpy(sp->buf + sp->len, buf, len);
  sp->len += len;
  sp->buf[sp->len] = '\0';
  return 1;
}

int /* append formatted string to sp (variadic) */
strbuf_addf(strbuf *sp, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int r = strbuf_addfv(sp, fmt, ap);
  va_end(ap);
  return r;
}

int /* append formatted string to sp (va_list) */
strbuf_addfv(strbuf *sp, const char *fmt, va_list ap)
{
  va_list aq;
  int chars;

  /* Make a copy of ap because we may traverse the list twice (if we
     need to grow the buffer); also notice that va_copy() requires a
     matching va_end(), and that the size argument to vsnprintf()
     includes the terminating \0, whereas its return value does not. */

  va_copy(aq, ap); /* C99 */
  chars = vsnprintf(sp->buf + sp->len, 0, fmt, aq);
  va_end(aq);

  if (chars < 0) return 0;
  if (!strbuf_ready(sp, chars)) return 0; /* nomem */

  chars = vsnprintf(sp->buf + sp->len, chars+1, fmt, ap);
  if (chars < 0) return 0;

  sp->len += chars;
  return 1;
}

void /* truncate string to exactly n <= len chars */
strbuf_trunc(strbuf *sp, size_t n)
{
  assert(sp != 0);
  if (!sp->buf) return; /* not allocated */
  assert(n <= sp->len); /* cannot expand */
  sp->len = n;
  sp->buf[n] = '\0';
}

int /* ensure enough space for dlen more characters */
strbuf_ready(strbuf *sp, size_t dlen)
{
  assert(sp != 0);
  /* nothing to do if allocated and enough room: */
  if (sp->buf && HASROOM(sp, dlen)) return 1;

  size_t requested = sp->len + dlen + 1; /* +1 for \0 */
  size_t standard = GROWFUNC(SIZE(sp));
  size_t newsize = MAX(requested, standard);

  newsize = (newsize+1)&~1; // round up to even
  char *ptr = realloc(sp->buf, newsize);
  if (!ptr) goto nomem;
  memset(ptr + sp->len, 0, newsize - sp->len);

  sp->buf = ptr;
  sp->size = newsize;
  return 1;

nomem:
  SETFAILED(sp);
  return 0;
}

void /* release memory, set to unallocated */
strbuf_free(strbuf *sp)
{
  assert(sp != 0);
  if (!sp->buf) return;
  free(sp->buf);
  sp->buf = 0;
  sp->len = sp->size = 0;
}
