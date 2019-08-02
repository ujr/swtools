/* strbuf.h - growable string buffer */

typedef struct strbuf {
  char *buf;    /* pointer to character buffer */
  size_t len;   /* string length (excluding terminating \0) */
  size_t size;  /* buffer size (even, including terminating \0) */
} strbuf;       /* invariants: len+1 <= size and always terminated */

/* The buffer size is always even; the thus redundant least significant bit
   serves as the failed flag (0=normal, 1=failed) */

#define strbuf_buffer(sp) ((sp)->buf)
#define strbuf_length(sp) ((sp)->buf ? (sp)->len : 0)
#define strbuf_size(sp) ((sp)->buf ? ((sp)->size & ~1) : 0)
#define strbuf_failed(sp) ((sp)->size & 1)

extern int strbuf_add(strbuf *sp, strbuf *sq);
extern int strbuf_addc(strbuf *sp, char c);
extern int strbuf_addz(strbuf *sp, const char *z);
extern int strbuf_addb(strbuf *sp, const char *buf, size_t len);
extern int strbuf_addf(strbuf *sp, const char *fmt, ...);
extern int strbuf_addfv(strbuf *sp, const char *fmt, va_list ap);

/* TODO add strbuf_trim(sp)? short names sbxxx()?*/

extern int strbuf_ready(strbuf *sp, size_t want);
extern void strbuf_trunc(strbuf *sp, size_t len);
extern void strbuf_free(strbuf *sp);

