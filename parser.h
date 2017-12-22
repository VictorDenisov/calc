#ifndef _PARSER_H_
#define _PARSER_H_

#define YYSTYPE CALC_STYPE
#define YYLTYPE CALC_LTYPE

#include "calc.tab.h"
#ifndef YY_DO_BEFORE_ACTION
#include "calc.lex.h"
#endif /* YY_DO_BEFORE_ACTION */

#endif /* _PARSER_H_ */
