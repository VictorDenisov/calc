#include "calc.h"
#include "calc.ast.h"
#define CALC_STYPE ast_node_t
#include "calc.parser.h"

#define CALC_RESULT(RHS) {			\
    expr_t * expr = calc_get_extra (scanner);	\
    expr->ast = RHS;				\
}

#define AST_BIN_OP(LHS, LEFT, OP, RIGHT) {	\
    LHS.bin_op.left = malloc (sizeof (LEFT));	\
    *LHS.bin_op.left = LEFT;			\
    LHS.bin_op.right = malloc (sizeof (RIGHT));	\
    *LHS.bin_op.right = RIGHT;			\
    LHS.token_type = OP;			\
  }

#define CALC_NUMBER(LHS, NUMBER) LHS = (ast_node_t){ .val = NUMBER.val, .token_type = TT_NUMBER, }
#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_PLUS, RIGHT)
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_MINUS, RIGHT)
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_MUL, RIGHT)
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) AST_BIN_OP (LHS, LEFT, TT_DIV, RIGHT)
static ast_node_t zero = { .val = 0, .token_type = TT_NUMBER, };
#define CALC_UN_MINUS(LHS, ARG) AST_BIN_OP (LHS, zero, TT_MINUS, ARG)
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG
#define CALC_X(LHS) LHS.token_type = TT_X

#define calc_parse calc_ast_parse
#include "calc.tab.c"

