#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <dlfcn.h>

#include "calc.h"
#include "cast_expr.h"

#define CHAR_PTR_TMPLT "%s"
#define CALC_FUNC_NAME "calc"
#define CALC_TMPLT__(CALC_TYPE_T) #CALC_TYPE_T " " CALC_FUNC_NAME " (" #CALC_TYPE_T " x) { return (" CHAR_PTR_TMPLT "); }\n"
#define CALC_TMPLT_(CALC_TYPE_T) CALC_TMPLT__ (CALC_TYPE_T)
#define CALC_TMPLT CALC_TMPLT_ (calc_type_t)

#define DEV_FD_TMPLT "/dev/fd/%d"
#define CC_TMPLT "cc -O2 -g -shared -fPIC -xc - -o " DEV_FD_TMPLT

typedef struct dl_state_t {
  calc_type_t (*compiled_func) (calc_type_t);
  void * dl;
} dl_state_t;

static parser_t calc_dl_init (char * expr)
{
  expr = cast_expr (expr);
  if (NULL == expr)
    goto fail;

  dl_state_t * dl_state = malloc (sizeof (*dl_state));
  if (NULL == dl_state)
    goto free_expr;

  int memfd = syscall (SYS_memfd_create, "shared.so", 0);
  if (memfd <= 0)
    {
      fprintf (stderr, "Failed to create memfd\n");
      goto free_state;
    }

  char cc_cmd[sizeof (CC_TMPLT) + sizeof ("65536")];
  sprintf (cc_cmd, CC_TMPLT, memfd);
  FILE * fd = popen (cc_cmd, "w");
  if (NULL == fd)
    {
      fprintf (stderr, "Failed to popen $CC\n");
      goto close_memfd;
    }
  
  int rv = fprintf (fd, CALC_TMPLT, expr);
  pclose (fd);

  if (strlen (expr) + sizeof (CALC_TMPLT) - sizeof (CHAR_PTR_TMPLT) != rv)
    {
      fprintf (stderr, "Failed to write code to the pipe\n");
      goto close_memfd;
    }
  
  char dev_fd[sizeof (DEV_FD_TMPLT) + sizeof ("65536")];
  sprintf (dev_fd, DEV_FD_TMPLT, memfd);
  dl_state->dl = dlopen (dev_fd, RTLD_LAZY);
  if (NULL == dl_state->dl)
    {
      fprintf (stderr, "Failed to dlopen\n");
      goto close_memfd;
    }

  close (memfd);
  
  dl_state->compiled_func = dlsym (dl_state->dl, CALC_FUNC_NAME);
  
  if (NULL == dl_state->compiled_func)
    goto free_state;

  free (expr);

  return (dl_state);

 close_memfd:
  close (memfd);
  
 free_state:
  free (dl_state);

 free_expr:
  free (expr);

 fail:
  return (NULL);
}

static void calc_dl_free (parser_t state)
{
  dl_state_t * dl_state = state;
  
  dlclose (dl_state->dl);

  free (dl_state);
}

static int calc_dl_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  dl_state_t * dl_state = state;
  *result = dl_state->compiled_func (arg_x->x);
  return (0);
}

parser_funcs_t dl_parser = {
  .init = calc_dl_init,
  .calc = calc_dl_calc,
  .free = calc_dl_free,
};
