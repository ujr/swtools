
#include <stddef.h>
#include <stdio.h>

struct lines {
  char *linebuf; /* buf.h */
  size_t *linepos; /* buf.h */
  size_t chunksize;
};

size_t appendline(char **buf, FILE *fp);
void truncline(char **buf);
void freeline(char **buf);

void clearlines(struct lines *plines);
int readlines(struct lines *plines, FILE *fp);
size_t countlines(struct lines *plines);
void writelines(struct lines *plines, FILE *fp);
void freelines(struct lines *plines);
