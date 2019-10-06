/* buf.h - growable memory buffers for C99
 *
 * Slightly modified from public domain (thank you!)
 * code at https://github.com/skeeto/growable-buf
 * 
 *   buf_size(v)      return number of elements in buffer (size_t)
 *   buf_capacity(v)  return capacity of buffer (size_t)
 *   buf_push(v, e)   append element e at end of buffer v
 *   buf_pop(v)       remove and return last element from v
 *   buf_clear(v)     clear buffer v (set its size to 0)
 *   buf_grow(v, n)   change buffer capacity by (ptrdiff_t) n elements
 *   buf_trunc(v, n)  set buffer capacity to (ptrdiff_t) n elements
 *   buf_free(v)      destroy and free the buffer
 *
 * Note: buf_{push,grow,trunc,free}() may change the buffer pointer;
 * copies of this pointer variable are thus invalidated!
 *
 * Usage:
 *   float *values = 0;       // zero-initialization is required!
 *   buf_push(values, 1.23);  // append (values may be realloc'ed)
 *   float f = values[0];     // access by ordinary indexing
 *   buf_free(values);        // release memory (changes values to 0)
 */

#pragma once

#include <stddef.h>
#include <stdlib.h>

#ifndef BUF_INIT_CAPACITY
#  define BUF_INIT_CAPACITY 8
#endif

#ifndef BUF_ABORT
#  define BUF_ABORT abort()
#endif

struct buf {
  size_t capacity;
  size_t size;
  char buffer[]; /* C99: flexible array member */
};

#define buf_ptr(v) /* C99: offsetof macro in stddef */ \
  ((struct buf *)((char *)(v) - offsetof(struct buf, buffer)))

#define buf_size(v) \
  ((v) ? buf_ptr(v)->size : 0)

#define buf_capacity(v) \
  ((v) ? buf_ptr(v)->capacity : 0)

#define buf_push(v, e) \
  do { \
    if (buf_capacity(v) == buf_size(v)) { \
      size_t n = buf_capacity(v) ? buf_capacity(v) : BUF_INIT_CAPACITY; \
      (v) = buf_grow1(v, sizeof(*(v)), n); \
    } \
    (v)[buf_ptr((v))->size++] = (e); \
  } while (0)

#define buf_pop(v) \
  ((v)[--buf_ptr(v)->size])

#define buf_clear(v) \
  ((v) ? (buf_ptr(v)->size = 0) : 0)

#define buf_grow(v, n) \
  ((v) = buf_grow1(v, sizeof(*(v)), n))

#define buf_trunc(v, n) \
  ((v) = buf_grow1(v, sizeof(*(v)), n - buf_capacity(v)))

#define buf_free(v) \
  do { \
    if (v) { \
      free(buf_ptr(v)); \
      (v) = 0; /* mark as unalloc'ed */ \
    } \
  } while (0)

static void *
buf_grow1(void *v, size_t esize, ptrdiff_t n)
{
  struct buf *bp;
  size_t max = ((size_t) -1) - sizeof(struct buf);

  if (v) {
    bp = buf_ptr(v);
    if (n > 0 && bp->capacity + n > max / esize)
      goto fail; /* overflow */
    bp = realloc(bp, sizeof(struct buf) + esize * (bp->capacity + n));
    if (!bp) goto fail; /* out of memory */
    bp->capacity += n;
    if (bp->size > bp->capacity)
      bp->size = bp->capacity;
  }
  else {
    if ((size_t) n > max / esize) goto fail; /* overflow */
    bp = malloc(sizeof(struct buf) + esize * n);
    if (!bp) goto fail; /* out of memory */
    bp->capacity = n;
    bp->size = 0;
  }

  return bp->buffer;
fail:
  BUF_ABORT;
  return 0;
}