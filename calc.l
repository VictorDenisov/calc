%option 8bit reentrant bison-bridge bison-locations
%option warn nounput never-interactive noyywrap
%option extra-type="void*"
%option prefix="calc_"

%{
#include <stdio.h>
#include "calc.h"
#include "calc.parser.h"

#define YY_NO_INPUT

static void calc_lloc (expr_t * expr, char * ptr)
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

#define YY_USER_ACTION ({                  \
      expr_t * expr = yyextra;             \
      calc_lloc (expr, yy_bp);             \
      yylloc->first_line = expr->lineno;   \
      yylloc->first_column = expr->column; \
      calc_lloc (expr, yy_cp);             \
      yylloc->last_line = expr->lineno;    \
      yylloc->last_column = expr->column;  \
    });

%}

INT_NUMBER [[:digit:]]+
FLOAT_NAN [Nn][Aa][Nn]
FLOAT_INF [Ii][Nn][Ff]
FLOAT_NUMBER {FLOAT_NAN}|{FLOAT_INF}|{INT_NUMBER}("."[[:digit:]]*)?([eE][+-]?{INT_NUMBER})?

%%
[' '] ;

{FLOAT_NUMBER} {
  *yylval = strtold (yytext, NULL);
  return NUMBER;
}

. {
  return yytext[0];
}
%%
