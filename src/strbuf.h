#pragma once
#ifndef STRBUF_H
#define STRBUF_H

/* Growable char buffer, always \0-terminated */

#include <stdarg.h>  /* va_list */
#include <stddef.h>  /* size_t */

typedef struct strbuf {
  char *buf;    /* pointer to character buffer */
  size_t len;   /* string length (excluding terminating \0) */
  size_t size;  /* buffer size (even, including terminating \0) */
} strbuf;       /* invariants: len+1 <= size and always terminated */

/* A strbuf is in one of three states: unallocated (buf==0,
   initial state), normal (buf!=0), failed (after a memory
   allocation failed).

   A strbuf MUST be initialized as in `strbuf sb = {0};`
   to start in the unallocated state! Alternatively, say
   `strbuf_init(&sb);` (but doing so on an allocated strbuf
   results in a memory leak).

   The buffer size is always even, its least significant bit
   thus always zero and terefore redundant; we use it as the
   failed flag (0=normal, 1=failed). */

#define strbuf_ptr(sp)     ((sp)->buf ? (sp)->buf : "")
#define strbuf_char(sp, i) ((sp)->buf[i])
#define strbuf_len(sp)     ((sp)->buf ? (sp)->len : 0)
#define strbuf_size(sp)    ((sp)->buf ? ((sp)->size & ~1) : 0)
#define strbuf_failed(sp)  ((sp)->size & 1)

int strbuf_add(strbuf *sp, strbuf *sq);
int strbuf_addc(strbuf *sp, int c);
int strbuf_addz(strbuf *sp, const char *z);
int strbuf_addb(strbuf *sp, const char *buf, size_t len);
int strbuf_addf(strbuf *sp, const char *fmt, ...);
int strbuf_addfv(strbuf *sp, const char *fmt, va_list ap);

void strbuf_init(strbuf *sp);
int strbuf_ready(strbuf *sp, size_t dlen);
void strbuf_trunc(strbuf *sp, size_t len);
void strbuf_free(strbuf *sp);

/* Define short names */

#ifndef STRBUF_NO_SHORT_NAMES
#define sbinit   strbuf_init
#define sbptr    strbuf_ptr
#define sbchar   strbuf_char
#define sblen    strbuf_len
#define sbsize   strbuf_size
#define sbfailed strbuf_failed
#define sbadd    strbuf_add
#define sbaddc   strbuf_addc
#define sbaddz   strbuf_addz
#define sbaddb   strbuf_addb
#define sbaddf   strbuf_addf
#define sbaddfv  strbuf_addfv
#define sbready  strbuf_ready
#define sbtrunc  strbuf_trunc
#define sbfree   strbuf_free
#endif

#endif
