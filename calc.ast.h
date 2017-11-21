#ifndef _CALC_AST_H_
#define _CALC_AST_H_

#include "yyscan.h"

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

extern int calc_ast_parse (yyscan_t scanner);

#endif /* _CALC_AST_H_ */
