#ifndef _AST_H_
#define _AST_H_

#include "calc.h"

typedef struct bin_op_t {
  unsigned int left;
  unsigned int right;
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
    calc_type_t val;
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

extern parser_funcs_t ast_parser_iter;
extern parser_funcs_t ast_parser_rec;

#endif /* _AST_H_ */
