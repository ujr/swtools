
#include "sorting.h"

static int /* default comparison function */
defaultcmp(int a, int b, void *dummy)
{
  (void) dummy; /* unused */
  return a < b ? -1 : a > b ? 1 : 0;
}


static void /* swap v[i] and v[j] */
swap(int v[], int i, int j)
{
  int t = v[i];
  v[i] = v[j];
  v[j] = t;
}


/* Bubble sort: section 4.1, simple but O(n^2) */

void /* sort v[0..n-1] */
bubblesort(int v[], int n)
{
  int i, j;
  for (i = n-1; i > 0; i--) {
    for (j = 0; j < i; j++) {
      if (v[j] > v[j+1]) { /* compare */
        swap(v, j, j+1);   /* swap */
      }
    }
  }
}


/* Shell sort: section 4.2, O(n^1.5), non-recursive */

void /* sort v[0..n-1] */
shellsort(int v[], int n)
{
  int gap, i, j;
  for (gap = n/2; gap > 0; gap /= 2) { /* shrink the gap */
    for (i = gap; i < n; i++) { /* iterate elements */
      for (j = i-gap; j >= 0; j -= gap) { /* ... */
        if (v[j] > v[j+gap]) { /* compare */
          swap(v, j, j+gap);   /* swap */
        }
        else break; /* v[j-gap] and v[j] already in order */
      }
    }
  }
}


/* Quick sort: section 4.4, O(n log n), worst case O(n^2), recursive */

static void /* sort v[lo..hi] */
quicksort2(int v[], int lo, int hi, int (*cmp)(int,int,void*), void *userdata)
{
  int i, last; /* indices into v[] */

  if (lo >= hi) return; /* nothing to sort */

  /* partition */
  swap(v, lo, (lo+hi)/2); /* use middle elem as pivot, place in v[lo] */
  last = lo;              /* v[lo..last-1] is the items < pivot */
  for (i = lo+1; i <= hi; i++)
    //if (v[i] < v[lo])     /* v[i] less than pivot? */
    if (cmp(v[i], v[lo], userdata) < 0) /* v[i] less than pivot? */
      swap(v, ++last, i); /* yes: swap into left subset */
  swap(v, lo, last);      /* restore pivot; we know v[last] <= v[lo] */

  /* recurse smaller subset first (to min stack depth) */
  if (last-lo < hi-last) {
    quicksort2(v, lo, last-1, cmp, userdata);
    quicksort2(v, last+1, hi, cmp, userdata);
  } else {
    quicksort2(v, last+1, hi, cmp, userdata);
    quicksort2(v, lo, last-1, cmp, userdata);
  }
}

void /* sort v[0..n-1] */
quicksort(int v[], int n, int (*cmp)(int,int,void*), void *userdata)
{
  if (!cmp) cmp = defaultcmp;
  quicksort2(v, 0, n-1, cmp, userdata);
}

void /* restore the heap property after heap[1] changed */
reheap(int heap[], int n, int (*cmp)(int,int,void*), void *userdata)
{
  int i = 1, j = 2*i;
  if (!cmp) cmp = defaultcmp;
  while (j <= n) {
    if (j < n) /* find smaller child */
      if (cmp(heap[j], heap[j+1], userdata) > 0) j += 1;
    if (cmp(heap[i], heap[j], userdata) <= 0) i = n; /* terminate loop */
    else swap(heap, i, j);
    i = j; j = 2*i;
  }
}
