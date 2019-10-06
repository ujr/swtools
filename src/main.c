
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
  { "translit", translitcmd },
  { "compare", comparecmd },
  { "include", includecmd },
  { "concat", concatcmd },
  { "print", printcmd },
  { "sort", sortcmd },
  { 0, 0 }
};

const char *me; // for error messages
int verbosity = 0;
static const char *progname;
static const char *toolname;

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
  fprintf(fp, "Usage: %s [options] <command> [arguments]\n"
              "   or: <command> [arguments]\n"
              " options:  -v  verbose mode\n"
              "           -h  show this help and quit\n"
              "           -V  show version and quit\n"
              " commands:", progname);
  for (int i=0; tools[i].name; i++)
    fprintf(fp, " %s", tools[i].name);
  fprintf(fp, "\n arguments are command-specific\n");
}

static int
parseopts(int argc, char **argv)
{
  int i, showhelp = 0, showversion = 0;

  for (i = 0; i < argc && argv[i]; i++) {
    const char *p = argv[i];
    if (*p != '-' || streq(p, "-")) break; // no more option args
    if (streq(p, "--")) { ++i; break; } // end of option args
    for (++p; *p; p++) {
      switch (*p) {
        case 'h': showhelp = 1; break;
        case 'v': verbosity += 1; break;
        case 'V': showversion = 1; break;
        default: usage("invalid option"); return -1;
      }
    }
  }

  if (showhelp || showversion) {
    identity(123);
    if (showhelp) usage(0);
    exit(SUCCESS);
  }

  return i; // #args parsed
}

int
main(int argc, char **argv)
{
  toolfun *cmd;
  int r;

  toolname = 0;
  progname = getprog(argv);
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
