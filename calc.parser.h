#ifndef _CALC_PARSER_H_
#define _CALC_PARSER_H_

#define CALC_STYPE long double
#define YYSTYPE CALC_STYPE
#define YYLTYPE CALC_LTYPE

#include "calc.tab.h"
#ifndef YY_DO_BEFORE_ACTION
#include "calc.lex.h"
#endif /* YY_DO_BEFORE_ACTION */

#endif /* _CALC_PARSER_H_ */
