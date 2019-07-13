
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"

struct {
  const char *name;
  toolfun *fun;
} tools[] = {
  { "copy", copycmd },
  { "echo", echocmd },
  { 0, 0 }
};

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
usage(const char *fmt, ...)
{
  char msg[256];
  va_list ap;
  if (fmt) {
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    fprintf(stderr, "%s: %s\n", progname, msg);
    va_end(ap);
  } else {
    fprintf(stderr, "This is %s version %s\n", progname, RELEASE);
  }
  fprintf(stderr, "Usage: %s <command> [arguments]\n", progname);
  fprintf(stderr, "   or: <command> [arguments]\n commands:");
  for (int i=0; tools[i].name; i++)
    fprintf(stderr, " %s", tools[i].name);
  fprintf(stderr, "\n arguments are command specific\n");
}

int
main(int argc, char **argv)
{
  toolfun *cmd;

  toolname = 0;
  progname = basename(argv);
  if (!progname) return FAILHARD;

  cmd = findtool(progname);
  if (cmd) {
    toolname = progname;
    return cmd(argc, argv);
  }

  argc -= 1, argv += 1; // shift progname

  if (!*argv) { // no command specified
    usage(0);
    return SUCCESS;
  }

  cmd = findtool(*argv);
  if (cmd) {
    toolname = *argv;
    return cmd(argc, argv);
  }

  usage("no such command: %s", *argv);
  return FAILHARD;
}
