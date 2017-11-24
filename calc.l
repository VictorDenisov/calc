%option 8bit reentrant bison-bridge bison-locations
%option warn nounput never-interactive noyywrap
%option extra-type="void*"
%option prefix="calc_"

%{
#include <stdio.h>
#define CALC_STYPE long double
#include "calc.h"
#include "calc.parser.h"

#define YY_NO_INPUT

static void calc_lloc (lex_loc_t * loc, char * ptr)
{
  if (NULL == loc->buf)
    {
      loc->buf = ptr;
      loc->offset = 0;
      loc->lineno = 1;
      loc->column = 0;
      return;
    }

  for ( ; &loc->buf[loc->offset] < ptr; ++loc->offset)
    if ('\n' == loc->buf[loc->offset])
      {
        ++loc->lineno;
        loc->column = 0;
      }
    else
      ++loc->column;
}

#define YY_USER_ACTION ({                 \
      lex_loc_t * loc = yyextra;          \
      calc_lloc (loc, yy_bp);             \
      yylloc->first_line = loc->lineno;   \
      yylloc->first_column = loc->column; \
      calc_lloc (loc, yy_cp);             \
      yylloc->last_line = loc->lineno;    \
      yylloc->last_column = loc->column;  \
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
