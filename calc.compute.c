#include "calc.h"
#include "calc.parser.h"

static void calc_result (yyscan_t scanner, YYSTYPE arg);

#define CALC_RESULT(RHS) calc_result (scanner, RHS)
#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) LHS = LEFT + RIGHT
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) LHS = LEFT - RIGHT
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) LHS = LEFT * RIGHT
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) LHS = LEFT / RIGHT
#define CALC_UN_MINUS(LHS, ARG) LHS = -ARG
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG
#define CALC_X(LHS) {                                                             \
  expr_t * expr = calc_get_extra (scanner);                                       \
  if (!expr->has_x)                                                               \
    {                                                                             \
      yyerror (&yylloc, scanner, YY_("x must be specified for this expression")); \
      YYABORT;                                                                    \
    }                                                                             \
  LHS = expr->x;                                                                  \
}

#include "calc.tab.c"

static void calc_result (yyscan_t scanner, YYSTYPE arg)
{
  expr_t * expr = calc_get_extra (scanner);
  expr->result = arg;
}

int calc_compute_parse (yyscan_t scanner)
{
  return calc_parse (scanner);
}
