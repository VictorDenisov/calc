#include "calc.h"
#include "calc.parser.h"

#define CALC_RESULT(RHS) {			\
  expr_t * expr = calc_get_extra (scanner);	\
  expr->result = RHS.val;			\
}

#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) LHS.val = LEFT.val + RIGHT.val
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) LHS.val = LEFT.val - RIGHT.val
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) LHS.val = LEFT.val * RIGHT.val
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) LHS.val = LEFT.val / RIGHT.val
#define CALC_UN_MINUS(LHS, ARG) LHS.val = -ARG.val
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG
#define CALC_X(LHS) {							\
    expr_t * expr = calc_get_extra (scanner);				\
    if (!expr->has_x)							\
      {									\
	yyerror (&yylloc, scanner, YY_("x must be specified for this expression")); \
	YYABORT;							\
      }									\
    LHS.val = expr->x;							\
  }

#define calc_parse calc_compute_parse_
#include "calc.tab.c"

int calc_compute_parse (yyscan_t scanner)
{
  return calc_compute_parse_ (scanner);
}
