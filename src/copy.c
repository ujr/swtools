
#include "common.h"

int
copycmd(int argc, char **argv)
{
  UNUSED(argc);
  UNUSED(argv);

  int c;
  while ((c = getch()) != EOF) {
    putch(c);
  }

  return checkioerr();
}
