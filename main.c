#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"

int main (int argc, char * argv[])
{
  expr_t expr;
  expr.has_x = false;

  if (getopt (argc, argv, "x:") == 'x')
    {
      char * end;
      expr.has_x = true;
      expr.x = strtold (optarg, &end);
      if (*end != '\0')
        {
          fprintf (stderr, "x must be a number (parse fail at %td in '%s')\n", end - optarg, optarg);
          return (EXIT_FAILURE);
        }
    }

  int buf_size = 1;
  int i;
  for (i = optind; i < argc; i++)
    buf_size += strlen (argv[i]) + 1;

  char buf[buf_size];
  char * ptr = buf;
  char * arg;
  for (i = optind; i < argc; ++i)
    {
      for (arg = argv[i]; *arg; )
        *ptr++ = *arg++;
      *ptr++ = ' ';
    }
  *ptr = 0;
  
  yyscan_t scanner;
  calc_lex_init_extra (&expr, &scanner);

  expr.buf = NULL;
  calc__scan_string (buf, scanner);
  int parse_result = calc_parse (scanner);
  calc_lex_destroy (scanner);

  if (parse_result != 0)
    {
      fprintf (stderr, "Failed to parse expression '%s'\n", buf);
      return (EXIT_FAILURE);
    }

  printf ("%Lg\n", expr.result);

  return (EXIT_SUCCESS);
}
