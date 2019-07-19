
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"

struct {
  const char *name;
  toolfun *fun;
} tools[] = {
  { "copy", copycmd },
  { "count", countcmd },
  { "detab", detabcmd },
  { "entab", entabcmd },
  { "echo", echocmd },
  { 0, 0 }
};

const char *me; // for error messages
int verbosity = 0;
static const char *progname;
static const char *toolname;

const char *
basename(char **argv)
{
  const char *s = 0;
  register const char *p;
  if (argv && *argv) {
    p = s = *argv;
    while (*p) {
      if (*p++ == '/') {
        if (p) s = p; // advance
      }
    }
  }
  return (char *) s;
}

int
scanint(const char *s, int *v)
{
  int neg, n;
  const char *p;

  if (!s) return 0;

  p = s;
  neg = 0;

  switch (*p) {
    case '-': neg=1; /* FALLTHRU */
    case '+': p+=1; break;
  }

  /* compute -n to get INT_MIN without overflow, but
     input outside INT_MIN..INT_MAX silently overflows */
  for (n = 0; isdigit(*p); p++) {
    n = 10 * n - (*p - '0');
  }

  if (v) *v = neg ? n : -n;
  return p - s; // #chars scanned
}

const char *
makeident(const char *s, const char *t)
{
  size_t ns = s ? strlen(s) : 0;
  size_t nt = t ? strlen(t) : 0;
  char *p = malloc(ns+1+nt+1);
  if (!p) abort(); // lazyness
  if (s) strcpy(p, s);
  if (s && t) p[ns] = ' ';
  if (t) strcpy(p+ns+1, t);
  return p;
}

/* print system error message to stderr */
void
printerr(const char *msg)
{
  FILE *fp = stderr;

  // Syntax: progname[ toolname]: [msg: ]errno\n

  fputs(progname, fp);

  if (toolname && *toolname && !streq(toolname, progname)) {
    fputc(' ', fp);
    fputs(toolname, fp);
  }

  if (msg && *msg) {
    fputs(": ", fp);
    fputs(msg, fp);
  }

  if (errno || !msg || !*msg) {
    fputs(": ", fp);
    fputs(strerror(errno), fp);
  }

  fputc('\n', fp);
}

static toolfun *
findtool(const char *name)
{
  if (!name) return 0; // no name
  for (int i=0; tools[i].name; i++){
    if (streq(name, tools[i].name)){
      return tools[i].fun;
    }
  }
  return 0; // not found
}

// Usage: quux [opts] <cmd> [args]
// Usage: <cmd> [args]

static void
identity()
{
  printf("This is %s version %s\n", progname, RELEASE);
}

static void
usage(const char *fmt, ...)
{
  char msg[256];
  va_list ap;
  FILE *fp = fmt ? stderr : stdout;
  if (fmt) {
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    fprintf(fp, "%s: %s\n", progname, msg);
    va_end(ap);
  }
  fprintf(fp, "Usage: %s [options] <command> [arguments]\n", progname);
  fprintf(fp, "   or: <command> [arguments]\n");
  fprintf(fp, " options:  -v  verbose mode\n commands:");
  for (int i=0; tools[i].name; i++)
    fprintf(fp, " %s", tools[i].name);
  fprintf(fp, "\n arguments are command specific\n");
}

static int
parseopts(int argc, char **argv)
{
  if (argc > 0 && argv[0] && *argv[0] == '-') {
    switch (argv[0][1]) {
      case 'v': verbosity += 1; break;
      case 'V': identity(0); exit(SUCCESS); break;
      default: usage("invalid option"); return -1;
    }
    return 1;
  }
  return 0;
}

int
main(int argc, char **argv)
{
  toolfun *cmd;
  int r;

  toolname = 0;
  progname = basename(argv);
  if (!progname) return FAILHARD;

  cmd = findtool(progname);
  if (cmd) {
    toolname = me = progname;
    return cmd(argc, argv);
  }

  SHIFTARGS(argc, argv, 1);
  r = parseopts(argc, argv);
  if (r < 0) return FAILHARD;
  SHIFTARGS(argc, argv, r);

  if (!*argv) { // no command specified
    identity();
    usage(0);
    return SUCCESS;
  }

  cmd = findtool(*argv);
  if (cmd) {
    toolname = *argv;
    me = makeident(progname, *argv);
    return cmd(argc, argv);
  }

  usage("no such command: %s", *argv);
  return FAILHARD;
}
