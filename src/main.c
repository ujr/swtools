
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"

struct {
  const char *name;
  toolfun *fun;
} tools[] = {
  { "echo", echocmd },
  { 0, 0 }
};

const char *
progname(char **argv){
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

static toolfun *
findtool(const char *name){
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

const char *me;
int verbosity = 0;

static void
usage(const char *fmt, ...)
{
  char msg[256];
  va_list ap;
  if (fmt) {
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    fprintf(stderr, "%s: %s\n", me, msg);
    va_end(ap);
  } else {
    fprintf(stderr, "This is %s version %s\n", me, RELEASE);
  }
  fprintf(stderr, "Usage: %s <command> [arguments]\n", me);
  fprintf(stderr, "   or: <command> [arguments]\n commands:");
  for (int i=0; tools[i].name; i++)
    fprintf(stderr, " %s", tools[i].name);
  fprintf(stderr, "\n arguments are command specific\n");
}

int
main(int argc, char **argv)
{
  toolfun *cmd;

  me = progname(argv);
  if (!me) return FAILHARD;

  cmd = findtool(me);
  if (cmd) return cmd(argc, argv);

  argc -= 1, argv += 1; // shift progname

  if (!*argv) {
    usage(0);
    return SUCCESS;
  }

  cmd = findtool(*argv);
  if (cmd) return cmd(argc, argv);

  usage("no such command: %s", *argv);
  return FAILHARD;
}
