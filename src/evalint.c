
#include <ctype.h>
#include <setjmp.h>
#include <stddef.h>

#include "common.h"
#include "eval.h"

/* Evaluation of simple integer expressions
** by recursive descent according to this grammar:
**
**   expr:   term (+|-) term
**   term:   factor (*|/|%) factor
**   factor: number | ( expr )
*/

#define LPAREN '('
#define RPAREN ')'
#define PLUS   '+'
#define MINUS  '-'
#define TIMES  '*'
#define DIVIDE '/'
#define MODULO '%'

struct eval {
  const char *s;
  size_t i;
  jmp_buf jmpbuf;
  int errcode;
  const char *errmsg;
};

static int expr(struct eval *ctx);
static int term(struct eval *ctx);
static int factor(struct eval *ctx);
static char peek(struct eval *ctx);
static void fail(struct eval *ctx, int code, const char *msg);

int evalint(const char *s, int *pval, const char **pmsg)
{
  struct eval ctx;
  int val;

  ctx.s = s ? s : "";
  ctx.i = 0;
  ctx.errcode = EVAL_OK;
  ctx.errmsg = 0;

  if (setjmp(ctx.jmpbuf)) {
    if (pval) *pval = 0;
    if (pmsg) *pmsg = ctx.errmsg;
    return ctx.errcode;
  }

  val = expr(&ctx);
  if (pval) *pval = val;
  if (pmsg) *pmsg = 0;
  return EVAL_OK;
}

static int expr(struct eval *ctx)
{
  int v = term(ctx);
  char c = peek(ctx);
  while (c == PLUS || c == MINUS) {
    ctx->i += 1;  /* skip operator */
    if (c == PLUS)
      v += term(ctx);
    else
      v -= term(ctx);
    c = peek(ctx);
  }
  return v;
}

static int term(struct eval *ctx)
{
  int w, v = factor(ctx);
  char c = peek(ctx);
  while (c == TIMES || c == DIVIDE || c == MODULO) {
    ctx->i += 1;  /* skip operator */
    w = factor(ctx);
    if (c == TIMES) v *= w;
    else {
      if (w == 0)
        fail(ctx, EVAL_DIVZERO, "division by zero");
      else if (c == DIVIDE) v /= w;
      else v %= w;
    }
    c = peek(ctx);
  }
  return v;
}

static int factor(struct eval *ctx)
{
  int v = 0;
  if (peek(ctx) == LPAREN) {
    ctx->i += 1;  /* skip paren */
    v = expr(ctx);
    if (peek(ctx) != RPAREN)
      fail(ctx, EVAL_SYNTAX, "missing right paren in expr");
    ctx->i += 1;  /* skip paren */
  }
  else {
    size_t n = scanint(ctx->s + ctx->i, &v);
    if (n) ctx->i += n;
    else fail(ctx, EVAL_SYNTAX, "expecting a number in expr");
  }
  return v;
}

/* peek: skip over white space, return next char */
static char peek(struct eval *ctx)
{
  while (isspace(ctx->s[ctx->i])) ctx->i += 1;
  return ctx->s[ctx->i];
}

static void fail(struct eval *ctx, int code, const char *msg)
{
  ctx->errcode = code;
  ctx->errmsg = msg;
  longjmp(ctx->jmpbuf, 1);
}
