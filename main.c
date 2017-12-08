#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.parser.h"
#include "calc.compute.h"
#include "calc.ast.h"
#include "calc.h"

typedef struct config_t {
  arg_x_t arg_x;
  char * expr;
} config_t;

bool parse_args (config_t * config, int argc, char * argv[])
{
  config->arg_x.has_x = false;

  if (getopt (argc, argv, "x:") == 'x')
    {
      char * end;
      config->arg_x.has_x = true;
      config->arg_x.x = strtold (optarg, &end);
      if (*end != '\0')
        {
          fprintf (stderr, "x must be a number (parse fail at %td in '%s')\n", end - optarg, optarg);
          return (false);
        }
    }

  int expr_size = 1;
  int i;
  for (i = optind; i < argc; i++)
    expr_size += strlen (argv[i]) + 1;

  config->expr = malloc (expr_size);
  if (NULL == config->expr)
    return (false);

  char * ptr = config->expr;
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

  return (true);
}

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
  config_t config;
  if (!parse_args (&config, argc, argv))
    return (EXIT_FAILURE);

  int rv_compute = run_calc (compute_parser, config.expr, config.arg_x);
  int rv_ast_rec = run_calc (ast_parser_rec, config.expr, config.arg_x);
  int rv_ast_iter = run_calc (ast_parser_iter, config.expr, config.arg_x);

  int rv = EXIT_SUCCESS;
  if (0 != rv_compute)
    rv = rv_compute;
  else if (0 != rv_ast_rec)
    rv = rv_ast_rec;
  else if (0 != rv_ast_iter)
    rv = rv_ast_iter;

  free (config.expr);
  return (rv);
}
