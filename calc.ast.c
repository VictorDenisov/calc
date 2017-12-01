#include <setjmp.h>

#include "calc.ast.h"
#include "calc.h"
#define CALC_STYPE struct ast_node_t
#include "calc.parser.h"

typedef struct bin_op_t {
  size_t left;
  size_t right;
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

typedef struct node_arena_t {
  ast_node_t * arena;
  unsigned int size;
  unsigned int allocated;
} node_arena_t;

typedef struct ast_state_t {
  node_arena_t arena;
} ast_state_t;

typedef struct ast_extra_t {
  lex_loc_t ll;
  node_arena_t * arena;
} ast_extra_t;

static int arena_init (node_arena_t * arena)
{
  arena->size = 4;
  arena->allocated = 1;
  arena->arena = malloc (sizeof (arena->arena[0]) * arena->size);
  if (NULL == arena->arena)
    return (!0);
  return (0);
}

static void arena_free (node_arena_t * arena)
{
  if (NULL != arena->arena)
    free (arena->arena);
}

static unsigned int allocate_nodes (node_arena_t * arena, unsigned int count)
{
  if (arena->allocated + count >= arena->size)
    {
      unsigned int new_size = 2 * arena->size;
      ast_node_t * new_arena = realloc (arena->arena, new_size * sizeof (arena->arena[0]));
      if (NULL == new_arena)
        return (0);
      arena->arena = new_arena;
      arena->size = new_size;
    }
  unsigned int result = arena->allocated;
  arena->allocated += count;
  return (result);
}

#define CALC_RESULT(RHS) {                          \
    ast_extra_t * extra = calc_get_extra (scanner); \
    extra->arena->arena[0] = RHS;                   \
}

#define AST_BIN_OP(LHS, LEFT, OP, RIGHT) {              \
    ast_extra_t * extra = calc_get_extra (scanner);     \
    node_arena_t * arena = extra->arena;                \
    unsigned int left = allocate_nodes (arena, 2);      \
    if (0 != left)                                      \
      {                                                 \
        arena->arena[left] = LEFT;                      \
        arena->arena[left + 1] = RIGHT;                 \
        LHS.bin_op.left = left;                         \
        LHS.bin_op.right= left + 1;                     \
      }                                                 \
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
  node_arena_t * arena;
} calc_ast_compute_args_t;

typedef enum calc_ast_compute_error_t {
  CACE_OK = 0,
  CACE_NOX,
} calc_ast_compute_error_t;

static long double calc_ast_compute (unsigned int index, calc_ast_compute_args_t * args)
{
  ast_node_t * node = &args->arena->arena[index];
  switch (node->token_type)
    {
    case TT_NUMBER:
      return (node->val);
    case TT_X:
      if (!args->arg_x.has_x)
        longjmp (args->error_env, CACE_NOX);
      return (args->arg_x.x);
    case TT_PLUS:
      return (calc_ast_compute (node->bin_op.left, args) + calc_ast_compute (node->bin_op.right, args));
    case TT_MINUS:
      return (calc_ast_compute (node->bin_op.left, args) - calc_ast_compute (node->bin_op.right, args));
    case TT_MUL:
      return (calc_ast_compute (node->bin_op.left, args) * calc_ast_compute (node->bin_op.right, args));
    case TT_DIV:
      return (calc_ast_compute (node->bin_op.left, args) / calc_ast_compute (node->bin_op.right, args));
    }
  return (0);
}

static int calc_ast_calc (parser_t state, arg_x_t * arg_x, long double * result)
{
  ast_state_t * ast = state;
  calc_ast_compute_args_t args = {
    .arg_x = *arg_x,
    .arena = &ast->arena,
  };
  int rv = setjmp (args.error_env);
  if (CACE_OK != rv)
    return (rv);
  *result = calc_ast_compute (0, &args);
  return (0);
}

static void calc_ast_free (parser_t state)
{
  ast_state_t * ast_state = state;
  arena_free (&ast_state->arena);
  free (ast_state);
}

static parser_t calc_ast_init (char * expr)
{
  int rv;

  ast_state_t * ast_state = malloc (sizeof (*ast_state));
  if (NULL == ast_state)
    return (NULL);
  rv = arena_init (&ast_state->arena);
  if (0 != rv)
    {
      free (ast_state);
      return (NULL);
    }

  ast_extra_t extra = {
    .ll = { .buf = NULL },
    .arena = &ast_state->arena,
  };

  yyscan_t scanner;
  rv = calc_lex_init_extra (&extra, &scanner);
  if (0 != rv)
    {
      calc_ast_free (ast_state);
      return (NULL);
    }

  calc__scan_string (expr, scanner);
  rv = calc_ast_parse (scanner);
  calc_lex_destroy (scanner);
  if (0 != rv)
    {
      calc_ast_free (ast_state);
      return (NULL);
    }

  return (ast_state);
}

parser_funcs_t ast_parser = {
  .init = calc_ast_init,
  .calc = calc_ast_calc,
  .free = calc_ast_free,
};
