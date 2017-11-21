#define CALC_STYPE long double
#include "calc.parser.h"
#include "calc.compute.h"
#include "calc.h"

#define CALC_RESULT(RHS) {			\
    expr_t * expr = calc_get_extra (scanner);	\
    expr->result = RHS;				\
}

#define CALC_NUMBER(LHS, NUMBER) LHS = NUMBER
#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) LHS = LEFT + RIGHT
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) LHS = LEFT - RIGHT
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) LHS = LEFT * RIGHT
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) LHS = LEFT / RIGHT
#define CALC_UN_MINUS(LHS, ARG) LHS = -ARG
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG
#define CALC_X(LHS) {							\
    expr_t * expr = calc_get_extra (scanner);				\
    if (!expr->has_x)							\
      {									\
	yyerror (&yylloc, scanner, YY_("x must be specified for this expression")); \
	YYABORT;							\
      }									\
    LHS = expr->x;							\
  }

#define calc_parse calc_compute_parse
#include "calc.tab.c"

