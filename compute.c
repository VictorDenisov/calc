#include "calc.h"
#define CALC_STYPE calc_type_t
#include "parser.h"
#include "compute.h"

typedef struct compute_state_t {
  char * expr;
  yyscan_t scanner;
} compute_state_t;

typedef struct compute_extra_t {
  lex_loc_t ll;
  arg_x_t arg_x;
  calc_type_t result;
} compute_extra_t;

#define CALC_RESULT(RHS) {                              \
    compute_extra_t * extra = calc_get_extra (scanner); \
    extra->result = RHS;                                \
}

#define CALC_NUM(LHS, NUMBER) LHS = NUMBER
#define CALC_ADD(LHS, LEFT, RIGHT) LHS = LEFT + RIGHT
#define CALC_SUB(LHS, LEFT, RIGHT) LHS = LEFT - RIGHT
#define CALC_MUL(LHS, LEFT, RIGHT) LHS = LEFT * RIGHT
#define CALC_DIV(LHS, LEFT, RIGHT) LHS = LEFT / RIGHT
#define CALC_NEG(LHS, ARG) LHS = -ARG
#define CALC_X(LHS) {                                             \
    compute_extra_t * extra = calc_get_extra (scanner);           \
    if (!extra->arg_x.has_x)                                      \
      {                                                           \
        yyerror (&yylloc, scanner,                                \
                 YY_("x must be specified for this expression")); \
        YYABORT;                                                  \
      }                                                           \
    LHS = extra->arg_x.x;                                         \
  }

#define calc_parse calc_compute_parse
#include "calc.tab.c"

static parser_t calc_compute_init (char * expr)
{
  compute_state_t * compute_state = malloc (sizeof (*compute_state));
  if (NULL == compute_state)
    return (NULL);
  compute_state->expr = expr;
  int rv = calc_lex_init (&compute_state->scanner);
  if (0 != rv)
    {
      free (compute_state);
      return (NULL);
    }

  return (compute_state);
}

static int calc_compute_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  compute_state_t * compute_state = state;
  compute_extra_t extra = {
    .ll = { .buf = NULL },
    .arg_x = *arg_x,
  };

  calc_set_extra (&extra, compute_state->scanner);

  calc__scan_string (compute_state->expr, compute_state->scanner);
  int parse_result = calc_compute_parse (compute_state->scanner);

  if (parse_result != 0)
    return (parse_result);

  *result = extra.result;
  return (0);
}

static void calc_compute_free (parser_t state)
{
  compute_state_t * compute_state = state;
  calc_lex_destroy (compute_state->scanner);
  free (compute_state);
}

parser_funcs_t compute_parser = {
  .init = calc_compute_init,
  .calc = calc_compute_calc,
  .free = calc_compute_free,
};
