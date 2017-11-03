%{
#include <stdio.h>
#include <string.h>

#include "calc.lex.h"

void calcerror (char * str)
{
  printf ("error\n");
}

int calcwrap ()
{
  return (1);
}

 int x = 0;
%}

%name-prefix "calc"
%file-prefix "calc"

%start str

%token NUMBER

%left '+' '-'
%left '*' '/'

%%
str: expr '\n'
{
  printf ("%d\n", $1);
}
;

expr: expr '+' expr
{
  $$ = $1 + $3;
}
|
expr '-' expr
{
  $$ = $1 - $3;
}
|
expr '*' expr
{
  $$ = $1 * $3;
}
|
expr '/' expr
{
  $$ = $1 / $3;
}
|
'(' expr ')'
{
  $$ = $2;
}
|
'x'
{
  $$ = x;
}
|
NUMBER
;
%%

int main (int argc, char* argv[])
{
  int buf_size = 2;
  int i;
  for (i = 1; i < argc; i++)
    {
      buf_size += strlen (argv[i]);
    }

  char buf[buf_size];
  char * ptr = buf;
  for (i = 1; i < argc; ++i)
    {
      char* arg;
      for (arg = argv[i]; *arg; )
	*ptr++ = *arg++;
    }
  *ptr++ = '\n';
  *ptr = 0;
  
  calc_scan_string(buf);
  calcparse ();
  
  x = 1;
  calc_scan_string(buf);
  calcparse ();
}
