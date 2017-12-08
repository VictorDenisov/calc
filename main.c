#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.parser.h"
#include "calc.compute.h"
#include "calc.ast.h"
#include "calc.h"

typedef enum parser_type_t {
  PT_COMPUTE,
  PT_AST_ITER,
  PT_AST_REC,
} parser_type_t;

typedef struct config_t {
  arg_x_t arg_x;
  char * expr;
  parser_type_t parser_type;
} config_t;

bool parse_args (config_t * config, int argc, char * argv[])
{
  config->arg_x.has_x = false;

  char * parser_types[] = {
    [PT_COMPUTE] = "compute",
    [PT_AST_ITER] = "ast_iter",
    [PT_AST_REC] = "ast_rec",
  };

  for (;;)
    {
      int opt = getopt (argc, argv, "p:x:");
      if (-1 == opt)
        break;

      switch (opt)
        {
        case 'p':
          {
            int i;
            int pt_len = sizeof (parser_types) / sizeof (parser_types[0]);
            for (i = 0; i < pt_len; ++i)
              if (0 == strcasecmp (optarg, parser_types[i]))
                {
                  config->parser_type = i;
                  break;
                }
            if (i >= pt_len)
              {
                fprintf (stderr, "Unknown parser type: %s\n", optarg);
                return (false);
              }
            break;
          }

        case 'x':
          {
            char * end;
            config->arg_x.has_x = true;
            config->arg_x.x = strtold (optarg, &end);
            if (*end != '\0')
              {
                fprintf (stderr, "x must be a number (parse fail at %td in '%s')\n", end - optarg, optarg);
                return (false);
              }
            break;
          }

        case '?':
          return (false);

        default:
          fprintf (stderr, "Unexpected result of getopt: %d ('%c')\n", opt, opt);
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

int run_calc (config_t * config)
{
  parser_funcs_t parser_funcs;
  switch (config->parser_type)
    {
    case PT_COMPUTE:
      parser_funcs = compute_parser;
      break;
    case PT_AST_ITER:
      parser_funcs = ast_parser_iter;
      break;
    case PT_AST_REC:
      parser_funcs = ast_parser_rec;
      break;
    default:
      fprintf (stderr, "Unexpected config->parser_type value: %d\n", config->parser_type);
      abort ();
    }

  long double result;

  parser_t parser = parser_funcs.init (config->expr);
  if (NULL == parser)
    {
      fprintf (stderr, "Failed to create parser\n");
      return (EXIT_FAILURE);
    }

  int rv = parser_funcs.calc (parser, &config->arg_x, &result);

  parser_funcs.free (parser);

  if (0 != rv)
    {
      fprintf (stderr, "Failed to calculate expression '%s'\n", config->expr);
      return (EXIT_FAILURE);
    }

  printf ("%Lg\n", result);
  return (EXIT_SUCCESS);
}

int main (int argc, char * argv[])
{
  config_t config = {
    .parser_type = PT_COMPUTE,
  };
  if (!parse_args (&config, argc, argv))
    return (EXIT_FAILURE);

  int rv = run_calc (&config);

  free (config.expr);
  return (rv);
}
