%{
#include <stdio.h>
#include "calc.tab.h"
%}

%option prefix="calc"
   
%%
[' '] ;

[0-9]+ {
  calclval = atoi (calctext);
  return NUMBER;
}

[^0-9] {
  return calctext[0];
}
%%
