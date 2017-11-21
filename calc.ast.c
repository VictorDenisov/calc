#include "calc.ast.h"
#include "calc.h"
#define CALC_STYPE ast_node_t
#include "calc.parser.h"

#define CALC_RESULT(RHS) {			\
    expr_t * expr = calc_get_extra (scanner);	\
    expr->ast = RHS;				\
}

#define AST_BIN_OP(LHS, LEFT, OP, RIGHT) {		\
    LHS.bin_op.left = malloc (sizeof (LEFT));		\
    if (LHS.bin_op.left)				\
      *LHS.bin_op.left = LEFT;				\
    LHS.bin_op.right= malloc (sizeof (RIGHT));		\
    if (LHS.bin_op.right)				\
      *LHS.bin_op.right = RIGHT;			\
    LHS.token_type = OP;				\
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

long double calc_ast_compute (ast_node_t * ast, long double x)
{
  switch (ast->token_type)
    {
    case TT_NUMBER:
      return (ast->val);
    case TT_X:
      return (x);
    case TT_PLUS:
      return (calc_ast_compute (ast->bin_op.left, x) + calc_ast_compute (ast->bin_op.right, x));
    case TT_MINUS:
      return (calc_ast_compute (ast->bin_op.left, x) - calc_ast_compute (ast->bin_op.right, x));
    case TT_MUL:
      return (calc_ast_compute (ast->bin_op.left, x) * calc_ast_compute (ast->bin_op.right, x));
    case TT_DIV:
      return (calc_ast_compute (ast->bin_op.left, x) / calc_ast_compute (ast->bin_op.right, x));
    }
  return (0);
}
