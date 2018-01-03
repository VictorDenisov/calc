#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "compute.h"
#include "ast.h"
#include "calc.h"
#include "gccjit.h"
#include "libjit.h"
#include "llvm.h"

#define DECIMAL (10)

typedef enum parser_type_t {
  PT_COMPUTE,
  PT_AST_ITER,
  PT_AST_REC,
  PT_GCCJIT,
  PT_LIBJIT,
  PT_LLVM,
} parser_type_t;

typedef struct config_t {
  arg_x_t arg_x;
  char * expr;
  parser_type_t parser_type;
  unsigned long iter_num;
} config_t;

bool parse_args (config_t * config, int argc, char * argv[])
{
  config->arg_x.has_x = false;

  char * parser_types[] = {
    [PT_COMPUTE] = "compute",
    [PT_AST_ITER] = "ast_iter",
    [PT_AST_REC] = "ast_rec",
    [PT_GCCJIT] = "gccjit",
    [PT_LIBJIT] = "libjit",
    [PT_LLVM] = "llvm",
  };

  for (;;)
    {
      int opt = getopt (argc, argv, "n:p:x:");
      if (-1 == opt)
        break;

      switch (opt)
        {
        case 'n':
          {
            char * end;
            config->iter_num = strtoul (optarg, &end, DECIMAL);
            if (*end != '\0')
              {
                fprintf (stderr, "n must be an integer number (parse fail at %td in '%s')\n", end - optarg, optarg);
                return (false);
              }
            break;
          }

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
    case PT_GCCJIT:
      parser_funcs = gccjit_parser;
      break;
    case PT_LIBJIT:
      parser_funcs = libjit_parser;
      break;
    case PT_LLVM:
      parser_funcs = llvm_parser;
      break;
    default:
      fprintf (stderr, "Unexpected config->parser_type value: %d\n", config->parser_type);
      abort ();
    }

  parser_t parser = parser_funcs.init (config->expr);
  if (NULL == parser)
    {
      fprintf (stderr, "Failed to create parser\n");
      return (EXIT_FAILURE);
    }

  calc_type_t sum = 0;
  int rv = 0;
  int i;
  for (i = 0; i < config->iter_num; ++i)
    {
      calc_type_t result;
      rv = parser_funcs.calc (parser, &config->arg_x, &result);
      if (0 != rv)
        break;
      sum += result;
    }

  parser_funcs.free (parser);

  if (0 != rv)
    {
      fprintf (stderr, "Failed to calculate expression '%s'\n", config->expr);
      return (EXIT_FAILURE);
    }

  static char * format[] = {
    [ __builtin_types_compatible_p (calc_type_t, __complex__ long double) ] = "%Lg",
    [ __builtin_types_compatible_p (calc_type_t, long double) ]             = "%Lg",
    [ __builtin_types_compatible_p (calc_type_t, double) ]                  = "%g",
    [ __builtin_types_compatible_p (calc_type_t, __complex__ double) ]      = "%g",
    [ __builtin_types_compatible_p (calc_type_t, float) ]                   = "%g",    
    [ __builtin_types_compatible_p (calc_type_t, __complex__ float) ]       = "%g",    
    [ __builtin_types_compatible_p (calc_type_t, long long) ]               = "%lld",    
    [ __builtin_types_compatible_p (calc_type_t, int) ]                     = "%d", /* default */
  };
  
  printf (format[sizeof (format) / sizeof (format[0]) - 1], sum);
  putchar ('\n');
  return (EXIT_SUCCESS);
}

int main (int argc, char * argv[])
{
  config_t config = {
    .parser_type = PT_COMPUTE,
    .iter_num = 1,
  };
  if (!parse_args (&config, argc, argv))
    return (EXIT_FAILURE);

  int rv = run_calc (&config);

  free (config.expr);
  return (rv);
}
