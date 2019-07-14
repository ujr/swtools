
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
  { "count", countcmd },
  { "echo", echocmd },
  { 0, 0 }
};

const char *me; // for error messages
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
  } else {
    fprintf(fp, "This is %s version %s\n", progname, RELEASE);
  }
  fprintf(fp, "Usage: %s <command> [arguments]\n", progname);
  fprintf(fp, "   or: <command> [arguments]\n commands:");
  for (int i=0; tools[i].name; i++)
    fprintf(fp, " %s", tools[i].name);
  fprintf(fp, "\n arguments are command specific\n");
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
    toolname = me = progname;
    return cmd(argc, argv);
  }

  SHIFTARGS(argc, argv, 1);

  if (!*argv) { // no command specified
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
