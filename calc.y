%{
#include <stdio.h>

#include "calc.h"

static void
calc_error (YYLTYPE * lloc, void * scanner, char * error)
{
  expr_t * expr = calc_get_extra (scanner);
  fprintf (stderr,
	   "%s\n"
	   "%*s^%*s^\n"
	   "%d:%d-%d:%d '%s'\n",
	   expr->buf,
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

%left '+' '-'
%left '*' '/'
%precedence NEG

%%
value: expr {
  expr_t * expr = calc_get_extra (scanner);
  expr->result = $1;
 };

expr: NUMBER
| expr '+' expr { $$ = $1 + $3; }
| expr '-' expr { $$ = $1 - $3; }
| expr '*' expr { $$ = $1 * $3; }
| expr '/' expr { $$ = $1 / $3; }
| '-' expr %prec NEG { $$ = - $2; }
| '+' expr %prec NEG { $$ = $2; }
| '(' expr ')' { $$ = $2; }
| 'x' {
  expr_t * expr = calc_get_extra (scanner);
  $$ = expr->x;
  };

%%

