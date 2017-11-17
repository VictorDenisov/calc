#ifndef _CALC_PARSER_H_
#define _CALC_PARSER_H_

typedef enum {
  NNT_VALUE,
  NNT_OP,
} nonterm_type_t;

typedef struct bin_op_t {
  struct nonterm_t * left;
  struct nonterm_t * right;
  char op;
} bin_op_t;

typedef struct nonterm_t {
  nonterm_type_t nonterm_type;
  union {
    long double val;
    bin_op_t bin_op;
  };
} nonterm_t;

#define CALC_STYPE nonterm_t
#define YYSTYPE CALC_STYPE
#define YYLTYPE CALC_LTYPE

#include "calc.tab.h"
#ifndef YY_DO_BEFORE_ACTION
#include "calc.lex.h"
#endif /* YY_DO_BEFORE_ACTION */

#endif /* _CALC_PARSER_H_ */
