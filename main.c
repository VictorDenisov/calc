#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.parser.h"
#include "calc.compute.h"
#include "calc.ast.h"
#include "calc.h"

int run_calc (parser_funcs_t parser_funcs, char * expr, arg_x_t arg_x)
{
  long double result;

  parser_t parser = parser_funcs.init (expr);
  if (NULL == parser)
    {
      fprintf (stderr, "Failed to create parser\n");
      return (EXIT_FAILURE);
    }

  int rv = parser_funcs.calc (parser, &arg_x, &result);

  parser_funcs.free (parser);

  if (0 != rv)
    {
      fprintf (stderr, "Failed to calculate expression '%s'\n", expr);
      return (EXIT_FAILURE);
    }

  printf ("%Lg\n", result);
  return (EXIT_SUCCESS);
}

int main (int argc, char * argv[])
{
  arg_x_t arg_x = { .has_x = false };

  if (getopt (argc, argv, "x:") == 'x')
    {
      char * end;
      arg_x.has_x = true;
      arg_x.x = strtold (optarg, &end);
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
  if (optind != argc)
    --ptr;
  *ptr = 0;

  int rv;
  rv = run_calc (compute_parser, buf, arg_x);
  if (0 != rv)
    return (rv);
  rv = run_calc (ast_parser, buf, arg_x);
  if (0 != rv)
    return (rv);

  return (EXIT_SUCCESS);
}
