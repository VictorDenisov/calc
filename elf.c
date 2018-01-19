#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <elf.h>

#include "calc.h"

//#define BIDIRECTIONAL_PIPE

#define CHAR_PTR_TMPLT "%s"
#define CALC_FUNC_NAME "calc"
#define CALC_TMPLT__(CALC_TYPE_T) #CALC_TYPE_T " " CALC_FUNC_NAME " (" #CALC_TYPE_T " x) { return (" CHAR_PTR_TMPLT "); }\n"
#define CALC_TMPLT_(CALC_TYPE_T) CALC_TMPLT__ (CALC_TYPE_T)
#define CALC_TMPLT CALC_TMPLT_ (calc_type_t)

#define READ_PIPE (0)
#define WRITE_PIPE (1)

typedef struct elf_state_t {
  calc_type_t (*compiled_func) (calc_type_t);
  void * map_addr;
  ssize_t map_size;
} elf_state_t;

static void *
dlsym (void * _elf, ssize_t size, char * symbol)
{
  char * elf = _elf;
  Elf64_Ehdr * header = _elf;

  if (offsetof (Elf64_Ehdr, e_shoff) + sizeof (((Elf64_Ehdr *) 0)->e_shoff) > size)
    {
      fprintf (stderr, "ELF is too short\n");
      return (NULL);
    }
  
  if ((ELFMAG0 != header->e_ident[EI_MAG0]) ||
      (ELFMAG1 != header->e_ident[EI_MAG1]) ||
      (ELFMAG2 != header->e_ident[EI_MAG2]) ||
      (ELFMAG3 != header->e_ident[EI_MAG3]))
    {
      fprintf (stderr, "ELF magic signature not found\n");
      return (NULL);
    }

  if (header->e_ident[EI_CLASS] != ELFCLASS64)
    {
      fprintf (stderr, "Unsupported ELF class\n");
      return (NULL);
    }

  Elf64_Shdr * sections = (Elf64_Shdr *)&elf[header->e_shoff];
  int dynsym = -1;
  int dynstr = -1;
  int i;
  
  if ((header->e_shnum <= 0) ||
      ((char*)&sections[header->e_shnum] > &elf[size]))
    {
      fprintf (stderr, "Invalid ELF format\n");
      return (NULL);
    }
  
  for (i = 0; i < header->e_shnum; ++i)
    if (sections[i].sh_type == SHT_DYNSYM)
      dynsym = i;
    else if ((sections[i].sh_type == SHT_STRTAB) &&
	     (sections[i].sh_flags & SHF_ALLOC))
      dynstr = i;

  if ((dynsym < 0) ||
      (dynstr < 0) ||
      (sections[dynsym].sh_offset < 0) || (sections[dynsym].sh_size < 0) ||
      (sections[dynstr].sh_offset < 0) || (sections[dynstr].sh_size < 0) ||
      (sections[dynsym].sh_offset + sections[dynsym].sh_size > size) ||
      (sections[dynstr].sh_offset + sections[dynstr].sh_size > size))
    {
      fprintf (stderr, "Failed to find symbols table in ELF\n");
      return (NULL);
    }

  Elf64_Sym * dynsym_tbl = (Elf64_Sym *)&elf[sections[dynsym].sh_offset];
  char * dynstr_tbl = &elf[sections[dynstr].sh_offset];
  size_t symbol_length = strlen (symbol) + 1;
  
  for (i = sections[dynsym].sh_size / sizeof (dynsym_tbl[0]) - 1; i >= 0; --i)
    if ((dynsym_tbl[i].st_name >= 0) &&
	(dynsym_tbl[i].st_name < sections[dynstr].sh_size) &&
	(symbol_length <= sections[dynstr].sh_size - dynsym_tbl[i].st_name) &&
	(0 == strcmp (symbol, &dynstr_tbl[dynsym_tbl[i].st_name])))
      {
	int text = dynsym_tbl[i].st_shndx;
	if ((text < 0) || (text >= header->e_shnum) ||
	    (sections[text].sh_offset < 0) || (sections[text].sh_size < 0) ||
	    (dynsym_tbl[i].st_value < 0) || (dynsym_tbl[i].st_size < 0) ||
	    (sections[text].sh_offset + sections[text].sh_size > size) ||
	    (sections[text].sh_offset + sections[text].sh_size <
	     dynsym_tbl[i].st_value + dynsym_tbl[i].st_size) ||
	    (dynsym_tbl[i].st_value < sections[text].sh_offset))

	  {
	    fprintf (stderr, "Invalid ELF format\n");
	    return (NULL);
	  }
	return (&elf[dynsym_tbl[i].st_value]);
      }
  fprintf (stderr, "Symbol '%s' nof found\n", symbol);
  return (NULL);
}

static parser_t calc_elf_init (char * expr)
{
  elf_state_t * elf_state = malloc (sizeof (*elf_state));
  if (NULL == elf_state)
    goto fail;

  int memfd = syscall (SYS_memfd_create, "shared.so", 0);
  if (memfd <= 0)
    {
      fprintf (stderr, "Failed to create memfd\n");
      goto free_state;
    }
  
  int rv;
  int p2c[2];

  rv = pipe (p2c);
  if (rv < 0)
    {
      fprintf (stderr, "Failed to create pipe from parent to child\n");
      goto close_memfd;
    }

#ifdef BIDIRECTIONAL_PIPE
  int c2p[2] = { -1, -1 };
  rv = pipe (c2p);
  if (rv < 0)
    {
      fprintf (stderr, "Failed to create pipe from child to parent\n");
      goto close_pipes;
    }
#endif /* BIDIRECTIONAL_PIPE */
  
  int cpid = fork ();
  if (cpid < 0)
    {
      fprintf (stderr, "Failed to fork\n");
      goto close_pipes;
    }    

  if (0 == cpid)
    {
      close (p2c[WRITE_PIPE]);
#ifdef BIDIRECTIONAL_PIPE
      close (c2p[READ_PIPE]);
      dup2 (c2p[WRITE_PIPE], STDOUT_FILENO);
#else /* !BIDIRECTIONAL_PIPE */
      dup2 (memfd, STDOUT_FILENO);
#endif /* BIDIRECTIONAL_PIPE */
      dup2 (p2c[READ_PIPE], STDIN_FILENO);

      rv = execl ("/usr/bin/cc", "cc", "-O2", "-g", "-shared", "-fPIC" , "-xc", "-", "-o/dev/stdout", NULL);
      if (rv < 0)
	{
	  fprintf (stderr, "Failed to exec cc;\n");
	  exit (EXIT_FAILURE);
	}
    }

  FILE * fd = fdopen (p2c[WRITE_PIPE], "w");
  if (NULL == fd)
    {
      fprintf (stderr, "Failed to make FILE * out of file descriptor\n");
      goto close_pipes;
    }
  
  rv = fprintf (fd, CALC_TMPLT, expr);
  fclose (fd);
  close (p2c[READ_PIPE]);
  
  p2c[WRITE_PIPE] = -1;
  p2c[READ_PIPE] = -1;
  
  if (strlen (expr) + sizeof (CALC_TMPLT) - sizeof (CHAR_PTR_TMPLT) != rv)
    {
      fprintf (stderr, "Failed to write code to the pipe\n");
      goto close_pipes;
    }
  
  waitpid (cpid, &rv, 0);
  if (0 != rv)
    {
      fprintf (stderr, "Compilation failed\n");
      goto close_pipes;
    }

#ifdef BIDIRECTIONAL_PIPE
  ssize_t pipe_size = fcntl (c2p[READ_PIPE], F_GETPIPE_SZ);
  if (pipe_size <= 0)
    {
      fprintf (stderr, "Unexpected pipe size %zd\n", pipe_size);
      goto close_pipes;
    }

  ssize_t size = 0;
  for (;;)
    {
      ssize_t chunk_size = splice (c2p[READ_PIPE], NULL, memfd, NULL, pipe_size, SPLICE_F_MOVE);
      if (chunk_size > 0)
	size += chunk_size;
      if (chunk_size != pipe_size)
	break;
    }
  close (c2p[WRITE_PIPE]);
  close (c2p[READ_PIPE]);
#else /* !BIDIRECTIONAL_PIPE */
  ssize_t size = lseek (memfd, 0, SEEK_END);
  if (size < 0)
    {
      fprintf (stderr, "Failed to lseek memfd\n");
      goto close_memfd;
    }
#endif /* BIDIRECTIONAL_PIPE */
  
  void * map_addr = mmap (NULL, size, PROT_EXEC | PROT_READ, MAP_PRIVATE, memfd, 0);
  if ((MAP_FAILED == map_addr) || (NULL == map_addr))
    {
      fprintf (stderr, "mmap failed\n");
      goto close_memfd;
    }
  
  elf_state->compiled_func = dlsym (map_addr, size, CALC_FUNC_NAME);
  
  if (NULL == elf_state->compiled_func)
    goto close_memfd;

  close (memfd);
  
  elf_state->map_addr = map_addr;
  elf_state->map_size = size;
  
  return (elf_state);

 close_pipes:
#ifdef BIDIRECTIONAL_PIPE
  if (c2p[WRITE_PIPE] > 0)
    close (c2p[WRITE_PIPE]);
  if (c2p[READ_PIPE] > 0)
    close (c2p[READ_PIPE]);
#endif /* BIDIRECTIONAL_PIPE */
  if (p2c[WRITE_PIPE] > 0)
    close (p2c[WRITE_PIPE]);
  if (p2c[READ_PIPE] > 0)
    close (p2c[READ_PIPE]);

 close_memfd:
  close (memfd);
  
 free_state:
  free (elf_state);
  
 fail:
  return (NULL);
}

static void calc_elf_free (parser_t state)
{
  elf_state_t * elf_state = state;
  
  munmap (elf_state->map_addr, elf_state->map_size);

  free (elf_state);
}

static int calc_elf_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  elf_state_t * elf_state = state;
  *result = elf_state->compiled_func (arg_x->x);
  return (0);
}

parser_funcs_t elf_parser = {
  .init = calc_elf_init,
  .calc = calc_elf_calc,
  .free = calc_elf_free,
};

      
