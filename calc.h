#ifndef _CALC_H_
#define _CALC_H_

#define CALC_STYPE long double
#define YYSTYPE CALC_STYPE
#define YYLTYPE CALC_LTYPE

#include "calc.tab.h"
#ifndef YY_DO_BEFORE_ACTION
#include "calc.lex.h"
#endif /* YY_DO_BEFORE_ACTION */

typedef struct expr_t {
  int lineno;
  int column;
  int offset;
  char * buf;
  long double x;
  long double result;
} expr_t;

static inline void calc_lloc (expr_t * expr, char * ptr)
{
  if (NULL == expr->buf)
    {
      expr->buf = ptr;
      expr->offset = 0;
      expr->lineno = 1;
      expr->column = 0;
      return;
    }
  
  for ( ; &expr->buf[expr->offset] < ptr; ++expr->offset)
    if ('\n' == expr->buf[expr->offset])
      {
	++expr->lineno;
	expr->column = 0;
      }
    else
      ++expr->column;
 }

#define YY_USER_ACTION ({			\
      expr_t * expr = yyextra;			\
      calc_lloc (expr, yy_bp);			\
      yylloc->first_line = expr->lineno;	\
      yylloc->first_column = expr->column;	\
      calc_lloc (expr, yy_cp);			\
      yylloc->last_line = expr->lineno;		\
      yylloc->last_column = expr->column;	\
    });

#endif /* _CALC_H_ */
