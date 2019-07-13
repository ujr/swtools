
#include "common.h"

int
echocmd(int argc, char **argv)
{
  const char blank = ' ';
  argc--, argv++; // shift
  // TODO options: -n, -e
  for (int i=0; i<argc; i++) {
    if (!*argv) break;
    if (i>0) putch(blank);
    putstr(*argv++);
  }
  putch('\n');
  return SUCCESS;
}
