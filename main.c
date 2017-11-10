#include <stdio.h>
#include <string.h>

#include "calc.h"

int main (int argc, char * argv[])
{
  int buf_size = 1;
  int i;
  for (i = 1; i < argc; i++)
    buf_size += strlen (argv[i]) + 1;

  char buf[buf_size];
  char * ptr = buf;
  char * arg;
  for (i = 1; i < argc; ++i)
    {
      for (arg = argv[i]; *arg; )
        *ptr++ = *arg++;
      *ptr++ = ' ';
    }
  *ptr = 0;
  
  yyscan_t scanner;
  expr_t expr;
  calc_lex_init_extra (&expr, &scanner);

  expr.x = 0;
  expr.buf = NULL;
  calc__scan_string (buf, scanner);
  calc_parse (scanner);
  printf ("result = %Lg\n", expr.result);

  expr.x = 1;
  expr.buf = NULL;
  calc__scan_string (buf, scanner);
  calc_parse (scanner);
  printf ("result = %Lg\n", expr.result);
  
  calc_lex_destroy (scanner);
}
