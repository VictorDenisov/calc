#ifndef _CALC_H_
#define _CALC_H_

#include <stdbool.h>

typedef struct lex_loc_t {
  int lineno;
  int column;
  int offset;
  char * buf;
} lex_loc_t;

typedef struct arg_x_t {
  long double x;
  bool has_x;
} arg_x_t;

typedef void * parser_t;

typedef struct parser_funcs_t {
  parser_t (*init) (char *);
  int (*calc) (parser_t, arg_x_t *, long double *);
  void (*free) (parser_t);
} parser_funcs_t;

#endif /* _CALC_H_ */
