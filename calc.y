%{
#include <stdio.h>

#include "calc.h"
#include "parser.h"
  
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
  term '+' expr { CALC_ADD ($$, $1, $3); }
| term '-' expr { CALC_SUB ($$, $1, $3); }
| term

term:
  factor '*' term { CALC_MUL ($$, $1, $3); }
| factor '/' term { CALC_DIV ($$, $1, $3); }
| factor

factor:
  NUMBER { CALC_NUM ($$, $1); }
| '-' factor { CALC_NEG ($$, $2); }
| '+' factor { $$ = $2; }
| '(' expr ')' { $$ = $2; }
| 'x' { CALC_X ($$); }

%%
