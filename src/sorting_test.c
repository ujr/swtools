/* Unit tests for sorting.c */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "test.h"
#include "sorting.h"

static void quickwrap(int v[], int n);
static void libcsort(int v[], int n);
static int qcompare(const void *p, const void *q);

static bool equals(int a[], int b[], int n);
static void reverse(int v[], int n);
static void print(int v[], int n);

typedef void (*sortproc)(int*,int);
static double bench(int a[], int n, int repeats, sortproc);

static int a[] = { 6, 10, 3, 2, 4, 8, 1, 7, 5, 9 };
static int b[] = { 6, 10, 3, 2, 4, 8, 1, 7, 5, 9 };
static int c[] = { 6, 10, 3, 2, 4, 8, 1, 7, 5, 9 };

static int r[] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }; // reverse sorted
static int z[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }; // sorted order

static int small[] = { 6, 10, 3, 2, 4, 8, 1, 7, 5, 9 };
static int large[1000] = {
  208, 836, 392, 483, 374, 508, 640, 820, 784, 705, 962, 106, 285, 558,
  397, 168, 525, 496, 765, 74, 476, 639, 105, 582, 86, 122, 189, 893,
  318, 1, 270, 46, 995, 973, 13, 430, 226, 272, 77, 78, 875, 788, 841,
  82, 702, 567, 947, 421, 977, 504, 681, 220, 787, 613, 920, 219, 938,
  967, 55, 147, 236, 514, 7, 656, 339, 332, 698, 435, 750, 799, 896, 239,
  626, 530, 294, 557, 869, 15, 786, 362, 633, 117, 718, 617, 395, 343,
  325, 335, 491, 80, 419, 487, 691, 694, 991, 506, 763, 291, 833, 234,
  986, 907, 123, 393, 494, 996, 564, 861, 478, 843, 682, 517, 166, 697,
  710, 372, 735, 966, 171, 700, 479, 742, 609, 952, 212, 323, 806, 747,
  842, 23, 761, 113, 865, 512, 965, 384, 851, 559, 205, 978, 279, 689,
  960, 982, 529, 127, 363, 449, 578, 375, 652, 140, 463, 218, 265, 5,
  96, 373, 989, 377, 139, 979, 985, 721, 447, 399, 424, 600, 534, 185,
  921, 75, 409, 796, 173, 992, 963, 674, 472, 298, 233, 4, 737, 507,
  344, 215, 897, 87, 543, 605, 515, 654, 401, 958, 437, 451, 566, 905,
  364, 327, 39, 937, 665, 840, 719, 26, 722, 775, 623, 745, 927, 572,
  957, 818, 338, 52, 88, 67, 935, 405, 110, 792, 221, 296, 99, 186,
  699, 133, 44, 794, 196, 317, 206, 404, 511, 891, 711, 40, 725, 590,
  179, 231, 326, 1000, 253, 910, 150, 188, 415, 307, 503, 941, 835,
  862, 746, 280, 498, 309, 262, 614, 361, 145, 165, 520, 961, 666,
  286, 257, 103, 888, 732, 797, 348, 36, 828, 442, 778, 857, 717, 177,
  583, 190, 917, 214, 997, 602, 886, 648, 826, 541, 853, 274, 62, 27,
  260, 187, 354, 657, 199, 807, 417, 293, 157, 467, 47, 125, 524, 315,
  933, 493, 158, 305, 756, 308, 631, 162, 43, 90, 61, 839, 555, 624,
  497, 538, 964, 906, 729, 677, 11, 6, 83, 195, 407, 100, 882, 432,
  108, 263, 845, 197, 670, 837, 68, 485, 673, 473, 72, 60, 528, 475,
  873, 155, 587, 102, 413, 51, 217, 649, 720, 426, 31, 12, 134, 770,
  380, 535, 831, 436, 181, 603, 767, 24, 84, 213, 141, 539, 929, 895,
  255, 586, 612, 601, 331, 885, 403, 247, 45, 191, 901, 183, 169, 368,
  249, 290, 988, 29, 320, 622, 469, 365, 726, 97, 874, 460, 713, 184,
  589, 757, 914, 551, 981, 651, 707, 519, 738, 19, 126, 59, 349, 571,
  360, 448, 371, 94, 310, 809, 176, 846, 474, 128, 121, 584, 898, 748,
  16, 949, 911, 751, 598, 625, 455, 444, 547, 367, 337, 819, 329, 685,
  143, 440, 716, 849, 41, 178, 366, 768, 306, 340, 781, 336, 18, 731,
  182, 854, 53, 500, 237, 505, 124, 378, 695, 295, 159, 457, 984, 245,
  880, 408, 876, 22, 936, 858, 112, 662, 570, 879, 313, 411, 146, 10,
  278, 892, 453, 878, 999, 73, 518, 940, 85, 232, 934, 254, 616, 610,
  316, 540, 248, 708, 532, 132, 994, 902, 634, 591, 548, 730, 79, 830,
  908, 38, 552, 161, 727, 667, 776, 388, 461, 637, 256, 462, 439, 352,
  370, 81, 658, 499, 834, 175, 553, 813, 386, 785, 822, 454, 704, 773,
  204, 943, 909, 416, 137, 49, 693, 758, 240, 456, 201, 3, 581, 932,
  333, 684, 881, 635, 774, 817, 838, 438, 537, 919, 70, 802, 412, 477,
  30, 203, 433, 297, 227, 741, 972, 660, 92, 17, 223, 509, 410, 894,
  696, 495, 282, 281, 56, 400, 678, 860, 268, 980, 855, 163, 154, 533,
  764, 250, 701, 28, 33, 101, 848, 621, 549, 592, 459, 71, 228, 795,
  322, 34, 193, 771, 21, 425, 69, 585, 620, 899, 754, 676, 847, 277,
  804, 261, 342, 844, 369, 381, 686, 490, 210, 556, 950, 466, 791, 351,
  114, 565, 312, 304, 951, 593, 743, 645, 266, 287, 93, 64, 715, 680,
  990, 889, 471, 345, 57, 619, 243, 641, 749, 930, 752, 723, 568, 431,
  324, 153, 608, 8, 492, 37, 604, 379, 224, 829, 303, 900, 544, 916,
  672, 264, 283, 887, 872, 942, 389, 252, 334, 200, 659, 734, 815, 759,
  780, 863, 939, 636, 434, 618, 611, 116, 521, 976, 703, 924, 464, 562,
  573, 668, 569, 800, 427, 762, 387, 803, 458, 89, 289, 144, 527, 859,
  968, 650, 890, 956, 513, 91, 242, 663, 48, 638, 488, 259, 194, 241,
  536, 870, 856, 644, 35, 606, 983, 627, 267, 160, 148, 643, 95, 877,
  687, 118, 918, 546, 594, 292, 753, 579, 76, 782, 760, 246, 953, 923,
  597, 357, 959, 998, 928, 414, 692, 420, 164, 580, 646, 98, 948, 311,
  66, 350, 883, 480, 390, 576, 207, 269, 805, 925, 915, 724, 653, 812,
  398, 690, 376, 465, 744, 180, 468, 284, 58, 225, 588, 510, 251, 482,
  628, 450, 728, 486, 446, 119, 974, 595, 2, 359, 300, 356, 823, 353,
  931, 174, 542, 502, 156, 655, 669, 501, 531, 561, 945, 811, 970, 871,
  299, 790, 868, 913, 814, 104, 714, 129, 563, 671, 25, 912, 428, 170,
  276, 441, 271, 975, 445, 50, 709, 661, 755, 969, 545, 273, 866, 679,
  120, 42, 192, 151, 810, 230, 944, 358, 852, 675, 319, 955, 789, 355,
  798, 130, 115, 149, 903, 832, 706, 222, 330, 993, 198, 481, 560, 769,
  607, 328, 642, 346, 172, 152, 550, 394, 202, 825, 347, 138, 470, 258,
  954, 216, 391, 107, 808, 32, 522, 423, 14, 135, 596, 54, 484, 526,
  275, 554, 142, 867, 683, 630, 238, 229, 341, 946, 321, 777, 443, 20,
  65, 577, 987, 647, 382, 109, 235, 288, 9, 418, 816, 629, 575, 733,
  884, 429, 779, 301, 712, 63, 821, 599, 574, 904, 827, 523, 385, 396,
  209, 211, 302, 824, 131, 783, 383, 739, 664, 422, 516, 615, 111, 926,
  971, 864, 402, 740, 167, 922, 244, 136, 736, 489, 801, 452, 406, 793,
  850, 688, 632, 772, 314, 766
};

void
sorting_test(int *pnumpass, int *pnumfail)
{
  int numpass = 0;
  int numfail = 0;

  int n = 10; /* array length */
  int m = 10*1000; /* repeat count for timings */
  double secs;

  HEADING("Testing Bubble Sort");
  bubblesort(a, n);
  TEST("sort 10 ints", equals(a, z, n));
  bubblesort(a, n);
  TEST("idempotent", equals(a, z, n));
  reverse(a, n);
  bubblesort(a, 0);
  TEST("sort empty", equals(a, r, n));

  HEADING("Testing Shell Sort");
  shellsort(b, n);
  TEST("sort 10 ints", equals(b, z, n));
  shellsort(b, n);
  TEST("idempotent", equals(b, z, n));
  reverse(b, n);
  shellsort(b, 0);
  TEST("sort empty", equals(b, r, n));

  HEADING("Testing Quick Sort");
  quicksort(c, n, 0, 0);
  TEST("sort 10 ints", equals(c, z, n));
  quicksort(c, n, 0, 0);
  TEST("idempotent", equals(c, z, n));
  reverse(c, n);
  quicksort(c, 0, 0, 0);
  TEST("sort empty", equals(c, r, n));

  HEADING("Comparing Bubble/Shell/Quick Sort");

  n = sizeof(small)/sizeof(small[0]);
  printf("Small array:"); print(small, n);

  secs = bench(small, n, m, bubblesort);
  INFO("%f secs to Bubble-sort small array (n=%d) m=%d times", secs, n, m);

  secs = bench(small, n, m, shellsort);
  INFO("%f secs to Shell-sort small array (n=%d) m=%d times", secs, n, m);

  secs = bench(small, n, m, quickwrap);
  INFO("%f secs to Quicksort small array (n=%d) m=%d times", secs, n, m);

  secs = bench(small, n, m, libcsort);
  INFO("%f secs to libc qsort small array (n=%d) m=%d times", secs, n, m);

  n = sizeof(large)/sizeof(large[0]);

  secs = bench(large, n, m, bubblesort);
  INFO("%f secs to Bubble-sort large array (n=%d) m=%d times", secs, n, m);

  secs = bench(large, n, m, shellsort);
  INFO("%f secs to Shell-sort large array (n=%d) m=%d times", secs, n, m);

  secs = bench(large, n, m, quickwrap);
  INFO("%f secs to Quicksort large array (n=%d) m=%d times", secs, n, m);

  secs = bench(large, n, m, libcsort);
  INFO("%f secs to libc qsort large array (n=%d) m=%d times", secs, n, m);

  HEADING("Testing reheap()");
  int heap[] = {0,1,2,3,4,5,6};
  reheap(heap, 6, 0, 0);
  int exph1[] = {0,1,2,3,4,5,6};
  TEST("h[1]=1; reheap", equals(exph1, heap, 7));
  heap[1] = 2;
  reheap(heap, 6, 0, 0);
  int exph2[] = {0,2,2,3,4,5,6};
  TEST("h[1]=2; reheap", equals(exph2, heap, 7));
  heap[1] = 9;
  reheap(heap, 6, 0, 0);
  int exph3[] = {0,2,4,3,9,5,6};
  TEST("h[1]=9; reheap", equals(exph3, heap, 7));
  heap[1] = 9;
  reheap(heap, 6, 0, 0);
  int exph4[] = {0,3,4,6,9,5,9};
  TEST("h[1]=9; reheap", equals(exph4, heap, 7));

  HEADING("Testing shuffle()");
  int v[] = {0,1,2,3,4,5};
  shuffle(v, 0, 0);
  TEST("empty", v[0] == 0);
  shuffle(v, 1, 0);
  TEST("singleton", v[0] == 0);
  shuffle(v, sizeof(v)/sizeof(v[0]), 0);
  INFO("shuffled: %d %d %d %d %d %d", v[0], v[1], v[2], v[3], v[4], v[5]);
  shuffle(v, sizeof(v)/sizeof(v[0]), 0);
  INFO("shuffled: %d %d %d %d %d %d", v[0], v[1], v[2], v[3], v[4], v[5]);

  if (pnumpass) *pnumpass += numpass;
  if (pnumfail) *pnumfail += numfail;
}

static bool /* return true iff a_i == b_i for i in 0..n-1 */
equals(int a[], int b[], int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (a[i] != b[i])
      return false;
  return true;
}

static void /* reverse v[0..n-1] */
reverse(int v[], int n)
{
  int i;
  for (i = 0, --n; i < n; i++, n--) {
    int t = v[i];
    v[i] = v[n];
    v[n] = t;
  }
}

static void /* print v[0..n-1] */
print(int v[], int n)
{
  int i;
  for (i = 0; i < n; i++)
    printf(" %d", v[i]);
  printf("\n");
}

static void
quickwrap(int v[], int n)
{
  quicksort(v, n, 0, 0);
}

static void /* wrap libc's qsort */
libcsort(int v[], int n)
{
  qsort(v, n, sizeof(int), qcompare);
}

static int /* compare function for qsort */
qcompare(const void *p, const void *q)
{
  int ip = * (int *) p;
  int iq = * (int *) q;
  return ip - iq;
}

static double
bench(int a[], int n, int repeats, void (*sorter)(int *, int))
{
  clock_t t0, t1;
  int i;

  int *b = calloc(n, sizeof(int));
  if (!b) abort();

  t0 = clock();
  for (i = 0; i < repeats; i++) {
    memcpy(b, a, n*sizeof(*b)); /* restore unsorted array */
    sorter(b, n); /* perform the sort */
  }
  t1 = clock();

  /*print(b, n > 10 ? 10 : n);*/

  free(b);

  return (double)(t1-t0)/CLOCKS_PER_SEC;
}

