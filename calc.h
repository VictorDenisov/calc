#ifndef _CALC_H_
#define _CALC_H_

#include <stdbool.h>

/* typedef long double calc_type_t; */
#ifndef calc_type_t
#define calc_type_t long double
#endif /* calc_type_t */

typedef struct lex_loc_t {
  int lineno;
  int column;
  int offset;
  char * buf;
} lex_loc_t;

typedef struct arg_x_t {
  calc_type_t x;
  bool has_x;
} arg_x_t;

typedef void * parser_t;

typedef struct parser_funcs_t {
  parser_t (*init) (char *);
  int (*calc) (parser_t, arg_x_t *, calc_type_t *);
  void (*free) (parser_t);
} parser_funcs_t;

#endif /* _CALC_H_ */
