#include <libgccjit.h>

#define CALC_STYPE union gccjit_rvalue_t
#include "parser.h"
#include "gccjit.h"
#include "calc.h"

typedef union gccjit_rvalue_t {
  calc_type_t val; /* lexer return value only */
  gcc_jit_rvalue * rvalue;
} gccjit_rvalue_t;

typedef struct gccjit_state_t {
  calc_type_t (*compiled_calc) (calc_type_t);
  gcc_jit_result * jit_result;
} gccjit_state_t;

typedef struct gccjit_extra_t {
  lex_loc_t ll;
  arg_x_t arg_x;
  gcc_jit_context * jit_ctx;
  gcc_jit_type * calc_type;
  gcc_jit_param * param_x;
} gccjit_extra_t;

void gccjit_result (gccjit_extra_t * extra, gcc_jit_rvalue * rhs) {
  gcc_jit_function * func =
    gcc_jit_context_new_function (extra->jit_ctx, NULL,
                                  GCC_JIT_FUNCTION_EXPORTED,
                                  extra->calc_type,
                                  "compiled_calc",
                                  1, &extra->param_x,
                                  0);
  gcc_jit_block * block = gcc_jit_function_new_block (func, NULL);
  gcc_jit_block_end_with_return (block, NULL, rhs);
}

#define CALC_RESULT(RHS) {                              \
    gccjit_extra_t * extra = calc_get_extra (scanner);  \
    gccjit_result (extra, RHS.rvalue);                  \
}

#define CALC_NUMBER(LHS, NUMBER) {					\
    gccjit_extra_t * extra = calc_get_extra (scanner);			\
    if (__builtin_types_compatible_p (calc_type_t, long double) ||	\
	__builtin_types_compatible_p (calc_type_t, __complex__ long double) || \
	__builtin_types_compatible_p (calc_type_t, double) ||		\
	__builtin_types_compatible_p (calc_type_t, __complex__ double) ||	\
	__builtin_types_compatible_p (calc_type_t, float) ||		\
	__builtin_types_compatible_p (calc_type_t, __complex__ float))	\
      LHS.rvalue = gcc_jit_context_new_rvalue_from_double (extra->jit_ctx, extra->calc_type, NUMBER.val); \
    else if (__builtin_types_compatible_p (calc_type_t, long) ||	\
	     __builtin_types_compatible_p (calc_type_t, unsigned long)) \
      LHS.rvalue = gcc_jit_context_new_rvalue_from_long (extra->jit_ctx, extra->calc_type, NUMBER.val); \
    else								\
      LHS.rvalue = gcc_jit_context_new_rvalue_from_int (extra->jit_ctx, extra->calc_type, NUMBER.val); \
  }

#define GCCJIT_BIN_OP(LHS, LEFT, OP, RIGHT) {          \
    gccjit_extra_t * extra = calc_get_extra (scanner); \
    LHS.rvalue = gcc_jit_context_new_binary_op (       \
        extra->jit_ctx,                                \
        NULL,                                          \
        OP,                                            \
        extra->calc_type,                              \
        LEFT.rvalue,                                   \
        RIGHT.rvalue);                                 \
}

#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) GCCJIT_BIN_OP (LHS, LEFT, GCC_JIT_BINARY_OP_PLUS, RIGHT)
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) GCCJIT_BIN_OP (LHS, LEFT, GCC_JIT_BINARY_OP_MINUS, RIGHT)
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) GCCJIT_BIN_OP (LHS, LEFT, GCC_JIT_BINARY_OP_MULT, RIGHT)
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) GCCJIT_BIN_OP (LHS, LEFT, GCC_JIT_BINARY_OP_DIVIDE, RIGHT)

#define CALC_UN_MINUS(LHS, ARG) {                      \
    gccjit_extra_t * extra = calc_get_extra (scanner); \
    LHS.rvalue = gcc_jit_context_new_unary_op (        \
        extra->jit_ctx,                                \
        NULL,                                          \
        GCC_JIT_UNARY_OP_MINUS,                        \
        extra->calc_type,                              \
        ARG.rvalue);                                   \
}
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG

#define CALC_X(LHS) {                                             \
    gccjit_extra_t * extra = calc_get_extra (scanner);            \
    LHS.rvalue = gcc_jit_param_as_rvalue (extra->param_x);        \
  }

#define calc_parse calc_gccjit_parse
#include "calc.tab.c"

static void calc_gccjit_free (parser_t state)
{
  gccjit_state_t * gccjit_state = state;
  if (NULL != gccjit_state->jit_result)
    gcc_jit_result_release (gccjit_state->jit_result);
  free (gccjit_state);
}

static parser_t calc_gccjit_init (char * expr)
{
  int rv;

  gccjit_state_t * gccjit_state = malloc (sizeof (*gccjit_state));
  if (NULL == gccjit_state)
    {
      fprintf (stderr, "Failed to allocate memory for parser state\n");
      goto fail;
    }
  gccjit_state->compiled_calc = NULL;
  gccjit_state->jit_result = NULL;

  gccjit_extra_t extra = {
    .ll = { .buf = NULL },
  };
  extra.jit_ctx = gcc_jit_context_acquire ();
  if (NULL == extra.jit_ctx)
    {
      fprintf (stderr, "Failed to acquire jit context\n");
      goto free_state;
    }

  yyscan_t scanner;
  rv = calc_lex_init_extra (&extra, &scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to initialize lexer\n");
      goto free_jit_ctx;
    }

  typeof (GCC_JIT_TYPE_LONG_DOUBLE) gcc_jit_type[] = {
    [ __builtin_types_compatible_p (calc_type_t, __complex__ long double) ] = GCC_JIT_TYPE_LONG_DOUBLE,
    [ __builtin_types_compatible_p (calc_type_t, long double) ]             = GCC_JIT_TYPE_LONG_DOUBLE,
    [ __builtin_types_compatible_p (calc_type_t, double) ]                  = GCC_JIT_TYPE_DOUBLE,
    [ __builtin_types_compatible_p (calc_type_t, __complex__ double) ]      = GCC_JIT_TYPE_DOUBLE,
    [ __builtin_types_compatible_p (calc_type_t, float) ]                   = GCC_JIT_TYPE_FLOAT,    
    [ __builtin_types_compatible_p (calc_type_t, __complex__ float) ]       = GCC_JIT_TYPE_FLOAT,    
    [ __builtin_types_compatible_p (calc_type_t, long long) ]               = GCC_JIT_TYPE_LONG_LONG,    
    [ __builtin_types_compatible_p (calc_type_t, int) ]                     = GCC_JIT_TYPE_INT, /* default */
  };
  extra.calc_type = gcc_jit_context_get_type (extra.jit_ctx, gcc_jit_type[sizeof (gcc_jit_type) / sizeof (gcc_jit_type[0]) - 1]);
  extra.param_x =
    gcc_jit_context_new_param (extra.jit_ctx, NULL, extra.calc_type, "x");

  calc__scan_string (expr, scanner);
  rv = calc_gccjit_parse (scanner);
  calc_lex_destroy (scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to parse\n");
      goto free_jit_ctx;
    }
  
  gccjit_state->jit_result = gcc_jit_context_compile (extra.jit_ctx);
  gcc_jit_context_release (extra.jit_ctx);
  if (NULL == gccjit_state->jit_result)
    {
      fprintf (stderr, "Failed to compile\n");
      goto free_state;
    }

  gccjit_state->compiled_calc =
    gcc_jit_result_get_code (gccjit_state->jit_result, "compiled_calc");
  if (NULL == gccjit_state->compiled_calc)
    {
      fprintf (stderr, "Failed to get compiled_calc function\n");
      goto free_state;
    }

  return (gccjit_state);

free_jit_ctx:
  gcc_jit_context_release (extra.jit_ctx);
free_state:
  calc_gccjit_free (gccjit_state);
fail:
  return (NULL);
}

static int calc_gccjit_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  gccjit_state_t * gccjit_state = state;
  *result = gccjit_state->compiled_calc (arg_x->x);
  return (0);
}

parser_funcs_t gccjit_parser = {
  .init = calc_gccjit_init,
  .calc = calc_gccjit_calc,
  .free = calc_gccjit_free,
};
