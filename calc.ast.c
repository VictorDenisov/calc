#include <setjmp.h>

#include "calc.ast.h"
#include "calc.h"
#define CALC_STYPE struct ast_node_t
#include "calc.parser.h"

typedef struct bin_op_t {
  struct ast_node_t * left;
  struct ast_node_t * right;
} bin_op_t;

typedef enum {
  TT_NUMBER,
  TT_X,
  TT_PLUS,
  TT_MINUS,
  TT_MUL,
  TT_DIV,
} token_type_t;

typedef struct ast_node_t {
  union {
    long double val;
    bin_op_t bin_op;
  };
  token_type_t token_type;
} ast_node_t;

typedef ast_node_t ast_state_t;

typedef struct ast_extra_t {
  lex_loc_t ll;
  ast_node_t ast;
} ast_extra_t;

#define CALC_RESULT(RHS) {                          \
    ast_extra_t * extra = calc_get_extra (scanner); \
    extra->ast = RHS;                               \
}

#define AST_BIN_OP(LHS, LEFT, OP, RIGHT) {              \
    LHS.bin_op.left = malloc (sizeof (LEFT));           \
    if (LHS.bin_op.left)                                \
      *LHS.bin_op.left = LEFT;                          \
    LHS.bin_op.right= malloc (sizeof (RIGHT));          \
    if (LHS.bin_op.right)                               \
      *LHS.bin_op.right = RIGHT;                        \
    LHS.token_type = OP;                                \
  }

#define CALC_NUMBER(LHS, NUMBER) LHS = (ast_node_t){ .val = NUMBER.val, .token_type = TT_NUMBER, }
#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_PLUS, RIGHT)
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_MINUS, RIGHT)
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_MUL, RIGHT)
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_DIV, RIGHT)
static ast_node_t zero = { .val = 0, .token_type = TT_NUMBER, };
#define CALC_UN_MINUS(LHS, ARG) AST_BIN_OP (LHS, zero, TT_MINUS, ARG)
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG
#define CALC_X(LHS)  LHS = (ast_node_t){ .val = 0, .token_type = TT_X, }

#define calc_parse calc_ast_parse
#include "calc.tab.c"

typedef struct calc_ast_compute_args_t {
  jmp_buf error_env;
  arg_x_t arg_x;
} calc_ast_compute_args_t;

typedef enum calc_ast_compute_error_t {
  CACE_OK = 0,
  CACE_NOX,
} calc_ast_compute_error_t;

static long double calc_ast_compute (ast_node_t * ast, calc_ast_compute_args_t * args)
{
  switch (ast->token_type)
    {
    case TT_NUMBER:
      return (ast->val);
    case TT_X:
      if (!args->arg_x.has_x)
        longjmp (args->error_env, CACE_NOX);
      return (args->arg_x.x);
    case TT_PLUS:
      return (calc_ast_compute (ast->bin_op.left, args) + calc_ast_compute (ast->bin_op.right, args));
    case TT_MINUS:
      return (calc_ast_compute (ast->bin_op.left, args) - calc_ast_compute (ast->bin_op.right, args));
    case TT_MUL:
      return (calc_ast_compute (ast->bin_op.left, args) * calc_ast_compute (ast->bin_op.right, args));
    case TT_DIV:
      return (calc_ast_compute (ast->bin_op.left, args) / calc_ast_compute (ast->bin_op.right, args));
    }
  return (0);
}

static int calc_ast_calc (parser_t state, arg_x_t * arg_x, long double * result)
{
  ast_node_t * ast = (ast_node_t *) state;
  calc_ast_compute_args_t args = {
    .arg_x = *arg_x,
  };
  int rv = setjmp (args.error_env);
  if (CACE_OK != rv)
    return (rv);
  *result = calc_ast_compute (ast, &args);
  return (0);
}

static void ast_node_free (ast_node_t * node);

static void ast_node_free_rec (ast_node_t * node) {
  if (NULL != node) {
    ast_node_free (node);
    free (node);
  }
}

static void ast_node_free (ast_node_t * node) {
  switch (node->token_type)
    {
    case TT_NUMBER:
    case TT_X:
      break;
    case TT_PLUS:
    case TT_MINUS:
    case TT_MUL:
    case TT_DIV:
      ast_node_free_rec (node->bin_op.left);
      ast_node_free_rec (node->bin_op.right);
      break;
    }
}

static parser_t calc_ast_init (char * expr)
{
  ast_state_t * ast_state = malloc (sizeof (*ast_state));
  if (NULL == ast_state)
    return (NULL);

  ast_extra_t extra = {
    .ll = { .buf = NULL },
  };

  yyscan_t scanner;
  int rv = calc_lex_init_extra (&extra, &scanner);
  if (0 != rv)
    {
      free (ast_state);
      return (NULL);
    }

  calc__scan_string (expr, scanner);
  rv = calc_ast_parse (scanner);
  calc_lex_destroy (scanner);
  if (0 != rv)
    {
      free (ast_state);
      return (NULL);
    }

  *ast_state = extra.ast;
  return (ast_state);
}

static void calc_ast_free (parser_t state)
{
  ast_state_t * ast_state = state;
  ast_node_free (ast_state);
  free (ast_state);
}

parser_funcs_t ast_parser = {
  .init = calc_ast_init,
  .calc = calc_ast_calc,
  .free = calc_ast_free,
};
