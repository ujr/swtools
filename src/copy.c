
#include "common.h"

static int
checkerr()
{
  if (ferror(stdin) || ferror(stdout)) {
    printerr(0);
    return FAILSOFT;
  }
  return SUCCESS;
}

int
copycmd(int argc, char **argv)
{
  UNUSED(argc);
  UNUSED(argv);

  int c;
  while ((c = getch()) != EOF) {
    putch(c);
  }

  return checkerr();
}
