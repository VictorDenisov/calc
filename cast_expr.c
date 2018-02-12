#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ast.h"

#define STR_(X) #X
#define STR(X) STR_ (X)
#define CALC_TYPE_T_CAST "(" STR (calc_type_t) ")"

typedef struct appendable_string_t {
  char * str;
  size_t size;
  size_t allocated;
} appendable_string_t;

static int __attribute__ ((format (printf, 2, 3)))
as_printf (appendable_string_t * appendable_string, const char * format, ...)
{
  va_list args;
  int length;
  char * str;
  size_t allocated = appendable_string->allocated;

  va_start (args, format);
  length = vasprintf (&str, format, args);
  va_end (args);
  if (NULL == str)
    goto out_of_memory;
  
  appendable_string->allocated += length;
  if (appendable_string->allocated > appendable_string->size)
    {
      while (appendable_string->allocated > appendable_string->size)
	appendable_string->size <<= 1;
      char * new_str = realloc (appendable_string->str, appendable_string->size);
      if (NULL == new_str)
	goto out_of_memory;
      appendable_string->str = new_str;
    }
    
  strcpy (&appendable_string->str[allocated - 1], str);
  free (str);
  
  return (length);

 out_of_memory:
  fprintf (stderr, "Out of memory\n");
  if (str)
    free (str);
  if (appendable_string->str)
    free (appendable_string->str);
  appendable_string->str = NULL;
  appendable_string->size = appendable_string->allocated = 0;
  return (-1);
}

static int print_ast_node (appendable_string_t * appendable_string, node_arena_t * arena, int index);

static int
print_bin_op (appendable_string_t * appendable_string, node_arena_t * arena, ast_node_t * node, char op)
{
  int rv, sum = 0;
  
  rv = as_printf (appendable_string, "(");
  if (rv < 0)
    return (rv);
  sum += rv;
  rv = print_ast_node (appendable_string, arena, node->bin_op.left);
  if (rv < 0)
    return (rv);
  sum += rv;
  rv = as_printf (appendable_string, " %c ", op);
  if (rv < 0)
    return (rv);
  sum += rv;
  rv = print_ast_node (appendable_string, arena, node->bin_op.right);
  if (rv < 0)
    return (rv);
  sum += rv;
  rv = as_printf (appendable_string, ")");
  if (rv < 0)
    return (rv);
  sum += rv;
  return (sum);
}

static int
print_casted_val (appendable_string_t * appendable_string, calc_type_t val)
{
  static char * format[] = {
    [ __builtin_types_compatible_p (calc_type_t, __complex__ long double) ] = "%.20Lg",
    [ __builtin_types_compatible_p (calc_type_t, long double) ]             = "%.20Lg",
    [ __builtin_types_compatible_p (calc_type_t, double) ]                  = "%.17g",
    [ __builtin_types_compatible_p (calc_type_t, __complex__ double) ]      = "%.17g",
    [ __builtin_types_compatible_p (calc_type_t, float) ]                   = "%.9g",
    [ __builtin_types_compatible_p (calc_type_t, __complex__ float) ]       = "%.9g",
    [ __builtin_types_compatible_p (calc_type_t, unsigned long long) ]      = "%llu",
    [ __builtin_types_compatible_p (calc_type_t, long long) ]               = "%lld",
    [ __builtin_types_compatible_p (calc_type_t, int) ]                     = "%d", /* default */
  };
  
  int rv = as_printf (appendable_string, CALC_TYPE_T_CAST);
  if (rv < 0)
    return (rv);
  return (as_printf (appendable_string, format[sizeof (format) / sizeof (format[0]) - 1], val));
}

static int
print_ast_node (appendable_string_t * appendable_string, node_arena_t * arena, int index)
{
  ast_node_t * node = &arena->arena[index];

  switch (node->token_type)
    {
    case TT_NUMBER:
      return (print_casted_val (appendable_string, node->val));
    case TT_X:
      return (as_printf (appendable_string, "x"));
    case TT_PLUS:
      return (print_bin_op (appendable_string, arena, node, '+'));
    case TT_MINUS:
      return (print_bin_op (appendable_string, arena, node, '-'));
    case TT_MUL:
      return (print_bin_op (appendable_string, arena, node, '*'));
    case TT_DIV:
      return (print_bin_op (appendable_string, arena, node, '/'));      
    }
  return (-1);
}

char * cast_expr (char * expr)
{
  ast_state_t * ast_state = ast_parser_iter.init (expr);
  if (NULL == ast_state)
    return (NULL);

  appendable_string_t appendable_string = {
    .str = NULL,
    .size = 1,
    .allocated = 1,
  };

  print_ast_node (&appendable_string, &ast_state->arena, ast_state->arena.allocated - 1);
  
  ast_parser_iter.free (ast_state);
  return (appendable_string.str);
}
