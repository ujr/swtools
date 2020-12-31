/* detab - convert tabs to blanks */

#include "common.h"

#define MAXSTOPS 100  /* TODO alloc dynamically */
#define TABSPACE 4    /* the default tab spacing */

static int tabinit(int stops[], int argc, char **argv);
static int tabstop(int stops[], int column);

int
detabcmd(int argc, char **argv)
{
  int r, c, col;
  int stops[MAXSTOPS];

  r = tabinit(stops, argc, argv);
  if (r != SUCCESS) return r;

  col = 0;
  while ((c = getch()) != EOF) {
    if (c == '\t') {
      int stop = tabstop(stops, col);
      for (; col < stop; col++) putch(' ');
    }
    else if (c == '\b') {
      putch(c); /* preserve the backspace */
      if (col > 0) col -= 1; /* but adjust column */
    }
    else if (c == '\n') {
      putch(c);
      col = 0;
    }
    else {
      putch(c);
      col += 1;
    }
  }

  return checkioerr();
}

int
entabcmd(int argc, char **argv)
{
  int r, c, col, newcol;
  int stops[MAXSTOPS];

  r = tabinit(stops, argc, argv);
  if (r != SUCCESS) return r;

  col = 0;
  do {
    newcol = col;
    while ((c = getch()) == ' ') { /* collect blanks */
      newcol += 1;
      if (newcol == tabstop(stops, col)) {
        putch('\t');
        col = newcol;
      }
    }
    if (c == '\t') {
      newcol = tabstop(stops, col);
      putch('\t');
      col = newcol;
    }
    else while (col < newcol) { /* remaining blanks */
      putch(' ');
      col += 1;
    }
    if (c != EOF && c != '\t') {
      putch(c);
      if (c == '\n') col = 0;
      else col += 1;
    }
  }
  while (c != EOF);

  return checkioerr();
}

/*
** Syntax: detab [t1 t2 ... tN]
** Each tab stop t_i is the column from left margin
** (or from the previous stop if given as +ti);
** the leftmost column is numbered 0 (not 1).
** If the last tab stop is prefixed with a + sign,
** then it is repeated indefinitely; for example,
** 2 +4 sets tabs in columns 2, 6, 10, 14, ...
** The tab stops must not be 0 and stricty increasing.
*/
static int
tabinit(int stops[], int argc, char **argv)
{
  int i, j, r, column, isrel;

  SHIFTARGS(argc, argv, 1);

  for (i=j=0, isrel=0; i<argc; i++) {
    if (!argv[i]) break;
    if (streq(argv[i], "--")) break;

    r = scanint(argv[i], &column);

    if (r <= 0 || argv[i][r] != '\0') {
      error("invalid tab stop argument");
      return FAILHARD;
    }
    if (column <= 0) {
      error("tab stop cannot be zero or negative");
      return FAILHARD;
    }
    if (j > 0 && column <= stops[j-1] && argv[i][0] != '+') {
      error("tab stops must be strictly increasing");
      return FAILHARD;
    }
    if (j+2 >= MAXSTOPS) {
      error("too many tab stops");
      return FAILHARD;
    }

    isrel = argv[i][0] == '+';
    if (isrel && j > 0) column += stops[j-1];
    stops[j++] = column;
  }

  if (j == 0) { /* no explicit stops, assume defaults */
    stops[j++] = TABSPACE;
  }
  else if (!isrel) { /* last not relative: add a +1 tab */
    stops[j] = stops[j-1] + 1;
    j++;
  }

  if (verbosity > 0) {
    fprintf(stderr, "%s: stops:", me);
    for(i=0;i<j;i++) fprintf(stderr, " %d", stops[i]);
    fprintf(stderr, "\n");
  }

  stops[j] = 0; /* end marker */
  return SUCCESS;
}

/* return first tab stop _after_ column */
static int
tabstop(int stops[], int column)
{
  int i = 0, r, s, o, d;
  while (0 < stops[i] && stops[i] <= column) i += 1;

  if (stops[i] > 0) return stops[i]; /* explicit stop */

  r = i > 1 ? stops[i-2] : 0; /* the second to last */
  s = i > 0 ? stops[i-1] : 0; /* and the last tab stop */

  o = s - r; /* tab spaces for repeating stop */
  d = column - s; /* distance to last stop */

  return s + (1+d/o)*o;
}
