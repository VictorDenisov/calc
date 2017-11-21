#ifndef _CALC_H_
#define _CALC_H_

#include <stdbool.h>

#include "calc.ast.h"

typedef struct expr_t {
  int lineno;
  int column;
  int offset;
  char * buf;
  long double x;
  bool has_x;
  long double result;
  ast_node_t ast;
} expr_t;

#endif /* _CALC_H_ */
