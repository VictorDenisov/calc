#include <jit/jit.h>

#define CALC_STYPE union libjit_rvalue_t
#include "parser.h"
#include "libjit.h"
#include "calc.h"

typedef union libjit_rvalue_t {
  calc_type_t val; /* lexer return value only */
  jit_value_t rvalue;
} libjit_rvalue_t;

typedef struct libjit_state_t {
  jit_context_t jit_ctx;
  jit_function_t func;
} libjit_state_t;

typedef struct libjit_extra_t {
  lex_loc_t ll;
  jit_function_t func;
  jit_value_t x;
} libjit_extra_t;

#define CALC_RESULT(RHS) {                              \
    libjit_extra_t * extra = calc_get_extra (scanner);  \
    jit_insn_return (extra->func, RHS.rvalue);          \
}

static const jit_type_t * _jit_calc_type[] = {
  [ __builtin_types_compatible_p (calc_type_t, __complex__ long double) ] = &jit_type_sys_long_double,
  [ __builtin_types_compatible_p (calc_type_t, long double) ]             = &jit_type_sys_long_double,
  [ __builtin_types_compatible_p (calc_type_t, double) ]                  = &jit_type_sys_double,
  [ __builtin_types_compatible_p (calc_type_t, __complex__ double) ]      = &jit_type_sys_double,
  [ __builtin_types_compatible_p (calc_type_t, float) ]                   = &jit_type_sys_float,
  [ __builtin_types_compatible_p (calc_type_t, __complex__ float) ]       = &jit_type_sys_float,
  [ __builtin_types_compatible_p (calc_type_t, long long) ]               = &jit_type_sys_longlong,
  [ __builtin_types_compatible_p (calc_type_t, int) ]                     = &jit_type_sys_int, /* default */
};
#define JIT_CALC_TYPE (*_jit_calc_type[sizeof (_jit_calc_type) / sizeof (_jit_calc_type[0]) - 1])

#define CALC_NUMBER(LHS, NUMBER) {                                      \
    libjit_extra_t * extra = calc_get_extra (scanner);                  \
    if (__builtin_types_compatible_p (calc_type_t, long double) ||      \
        __builtin_types_compatible_p (calc_type_t, __complex__ long double) || \
        __builtin_types_compatible_p (calc_type_t, double) ||           \
        __builtin_types_compatible_p (calc_type_t, __complex__ double) ||       \
        __builtin_types_compatible_p (calc_type_t, float) ||            \
        __builtin_types_compatible_p (calc_type_t, __complex__ float))  \
      LHS.rvalue = jit_value_create_nfloat_constant (extra->func, JIT_CALC_TYPE, NUMBER.val); \
    else                                                                \
      LHS.rvalue = jit_value_create_nint_constant (extra->func, JIT_CALC_TYPE, NUMBER.val); \
  }

#define LIBJIT_BIN_OP(LHS, LEFT, OP, RIGHT) {                 \
    libjit_extra_t * extra = calc_get_extra (scanner);        \
    LHS.rvalue = jit_insn_ ## OP (extra->func, LEFT.rvalue, RIGHT.rvalue); \
}

#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) LIBJIT_BIN_OP (LHS, LEFT, add, RIGHT)
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) LIBJIT_BIN_OP (LHS, LEFT, sub, RIGHT)
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) LIBJIT_BIN_OP (LHS, LEFT, mul, RIGHT)
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) LIBJIT_BIN_OP (LHS, LEFT, div, RIGHT)

#define CALC_UN_MINUS(LHS, ARG) {                        \
    libjit_extra_t * extra = calc_get_extra (scanner);   \
    LHS.rvalue = jit_insn_neg (extra->func, ARG.rvalue); \
}
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG

#define CALC_X(LHS) {                                  \
    libjit_extra_t * extra = calc_get_extra (scanner); \
    LHS.rvalue = extra->x;                             \
  }

#define calc_parse calc_libjit_parse
#include "calc.tab.c"

static void calc_libjit_free (parser_t state)
{
  libjit_state_t * libjit_state = state;
  if (NULL != libjit_state->func)
    jit_function_abandon (libjit_state->func);
  if (NULL != libjit_state->jit_ctx)
    jit_context_destroy (libjit_state->jit_ctx);
  free (libjit_state);
}

#define ALLOC_FAIL(VAR, TEXT)                                       \
  if (NULL == VAR)                                                  \
    {                                                               \
      fprintf (stderr, "Failed to allocate memory for " TEXT "\n"); \
      goto free_state;                                              \
    }

static parser_t calc_libjit_init (char * expr)
{
  int rv;

  libjit_state_t * libjit_state = malloc (sizeof (*libjit_state));
  if (NULL == libjit_state)
    {
      fprintf (stderr, "Failed to allocate memory for parser state\n");
      goto fail;
    }
  libjit_state->func = NULL;
  libjit_state->jit_ctx = jit_context_create ();
  ALLOC_FAIL (libjit_state->jit_ctx, "JIT context");

  libjit_extra_t extra = {
    .ll = { .buf = NULL },
  };
  jit_context_build_start (libjit_state->jit_ctx);

  jit_type_t func_arg_types[1] = { JIT_CALC_TYPE };
  jit_type_t signature = jit_type_create_signature (
    jit_abi_cdecl,
    JIT_CALC_TYPE,
    func_arg_types, /* num_params = */ 1,
    /* incref = */ 1);
  ALLOC_FAIL (signature, "function signature");
  libjit_state->func = extra.func = jit_function_create (libjit_state->jit_ctx, signature);
  ALLOC_FAIL (extra.func, "function object");
  jit_type_free (signature);
  extra.x = jit_value_get_param (extra.func, 0);
  ALLOC_FAIL (extra.x, "function argument");

  yyscan_t scanner;
  rv = calc_lex_init_extra (&extra, &scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to initialize lexer\n");
      goto free_state;
    }

  calc__scan_string (expr, scanner);
  rv = calc_libjit_parse (scanner);
  calc_lex_destroy (scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to parse\n");
      goto free_state;
    }

  rv = jit_function_compile (libjit_state->func);
  jit_context_build_end (libjit_state->jit_ctx);
  if (0 == rv)
    {
      fprintf (stderr, "Failed to compile function\n");
      goto free_state;
    }

  return (libjit_state);

free_state:
  calc_libjit_free (libjit_state);
fail:
  return (NULL);
}

static int calc_libjit_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  libjit_state_t * libjit_state = state;
  void *args[] = { &arg_x->x };
  int rv = jit_function_apply (libjit_state->func, args, result);
  return (!rv);
}

parser_funcs_t libjit_parser = {
  .init = calc_libjit_init,
  .calc = calc_libjit_calc,
  .free = calc_libjit_free,
};
