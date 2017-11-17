%{
#include <stdio.h>

#include "calc.h"
#include "calc.parser.h"

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

%%
value: expr {
  expr_t * expr = calc_get_extra (scanner);
  expr->result = $1;
 };

expr:
  term '+' term { $$ = $1 + $3; }
| term '-' term { $$ = $1 - $3; }
| term { $$ = $1; }

term:
  factor '*' factor { $$ = $1 * $3; }
| factor '/' factor { $$ = $1 / $3; }
| factor { $$ = $1; }

factor:
  NUMBER
| '-' factor { $$ = - $2; }
| '+' factor { $$ = $2; }
| '(' expr ')' { $$ = $2; }
| 'x' {
  expr_t * expr = calc_get_extra (scanner);
  if (!expr->has_x)
    {
      yyerror (&yylloc, scanner, YY_("x must be specified for this expression"));
      YYABORT;
    }
  $$ = expr->x;
  };

%%
