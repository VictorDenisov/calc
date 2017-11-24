%{
#include <stdio.h>

#include "calc.h"
#ifndef CALC_STYPE
#define CALC_STYPE long double
#endif /* CALC_STYPE */
#include "calc.parser.h"
  
static void
calc_error (YYLTYPE * lloc, void * scanner, char * error)
{
  lex_loc_t * loc = calc_get_extra (scanner);
  fprintf (stderr,
	   "%s\n"
	   "%*s^%*s^\n"
	   "%d:%d-%d:%d '%s'\n",
	   loc->buf,
	   lloc->first_column, "", lloc->last_column - lloc->first_column - 1, "",
	   lloc->first_line, lloc->first_column, lloc->last_line, lloc->last_column, error);
}
  
%}

%define api.prefix {calc_}
%define api.pure full
%param {void * scanner}
%locations
%defines

%start value

%token NUMBER

%%
value: expr { CALC_RESULT ($1); }

expr:
  term '+' expr { CALC_BIN_PLUS ($$, $1, $3); }
| term '-' expr { CALC_BIN_MINUS ($$, $1, $3); }
| term

term:
  factor '*' term { CALC_BIN_MUL ($$, $1, $3); }
| factor '/' term { CALC_BIN_DIV ($$, $1, $3); }
| factor

factor:
  NUMBER { CALC_NUMBER ($$, $1); }
| '-' factor { CALC_UN_MINUS ($$, $2); }
| '+' factor { CALC_UN_PLUS ($$, $2); }
| '(' expr ')' { $$ = $2; }
| 'x' { CALC_X ($$); }

%%
