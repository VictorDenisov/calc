%option 8bit reentrant bison-bridge bison-locations
%option warn nounput never-interactive noyywrap
%option extra-type="void*"
%option prefix="calc_"
   
%{
#include <stdio.h>
#include "calc.h"
   
#define YY_NO_INPUT
%}

INT_NUMBER [+-]?[[:digit:]]+
FLOAT_NAN [+-]?[Nn][Aa][Nn]
FLOAT_INF [+-]?[Ii][Nn][Ff]
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
