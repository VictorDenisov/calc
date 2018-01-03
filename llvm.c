#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/OrcBindings.h>

#define CALC_STYPE union llvm_rvalue_t
#include "parser.h"
#include "llvm.h"
#include "calc.h"

typedef union llvm_rvalue_t {
  calc_type_t val; /* lexer return value only */
  LLVMValueRef rvalue;
} llvm_rvalue_t;

typedef struct llvm_state_t {
  LLVMOrcJITStackRef orc_ref;
  calc_type_t (*compiled_func) (calc_type_t);
} llvm_state_t;

typedef struct llvm_extra_t {
  lex_loc_t ll;
  LLVMBuilderRef builder;
  LLVMValueRef x;
} llvm_extra_t;

#define CALC_RESULT(RHS) {                            \
    llvm_extra_t * extra = calc_get_extra (scanner);  \
    LLVMBuildRet (extra->builder, RHS.rvalue);        \
}

static LLVMTypeRef (* _llvm_calc_type[])() = {
  [ __builtin_types_compatible_p (calc_type_t, __complex__ long double) ] = LLVMX86FP80Type,
  [ __builtin_types_compatible_p (calc_type_t, long double) ]             = LLVMX86FP80Type,
  [ __builtin_types_compatible_p (calc_type_t, double) ]                  = LLVMDoubleType,
  [ __builtin_types_compatible_p (calc_type_t, __complex__ double) ]      = LLVMDoubleType,
  [ __builtin_types_compatible_p (calc_type_t, float) ]                   = LLVMFloatType,
  [ __builtin_types_compatible_p (calc_type_t, __complex__ float) ]       = LLVMFloatType,
  [ __builtin_types_compatible_p (calc_type_t, long long) ]               = LLVMInt64Type,
  [ __builtin_types_compatible_p (calc_type_t, int) ]                     = LLVMInt32Type, /* default */
};
#define LLVM_CALC_TYPE (_llvm_calc_type[sizeof (_llvm_calc_type) / sizeof (_llvm_calc_type[0]) - 1]())

#define CALC_NUMBER(LHS, NUMBER) {                                                  \
    if (__builtin_types_compatible_p (calc_type_t, long double) ||                  \
        __builtin_types_compatible_p (calc_type_t, __complex__ long double) ||      \
        __builtin_types_compatible_p (calc_type_t, double) ||                       \
        __builtin_types_compatible_p (calc_type_t, __complex__ double) ||           \
        __builtin_types_compatible_p (calc_type_t, float) ||                        \
        __builtin_types_compatible_p (calc_type_t, __complex__ float))              \
      LHS.rvalue = LLVMConstReal (LLVM_CALC_TYPE, NUMBER.val);                      \
    else                                                                            \
      LHS.rvalue = LLVMConstInt  (LLVM_CALC_TYPE, NUMBER.val, /* SignExtend = */1); \
  }

#define LLVM_BIN_OP(LHS, LEFT, OP, RIGHT) {                                       \
    llvm_extra_t * extra = calc_get_extra (scanner);                              \
    LHS.rvalue = LLVMBuild ## OP (extra->builder, LEFT.rvalue, RIGHT.rvalue, ""); \
}

#define CALC_BIN_PLUS(LHS, LEFT, RIGHT) LLVM_BIN_OP (LHS, LEFT, FAdd, RIGHT)
#define CALC_BIN_MINUS(LHS, LEFT, RIGHT) LLVM_BIN_OP (LHS, LEFT, FSub, RIGHT)
#define CALC_BIN_MUL(LHS, LEFT, RIGHT) LLVM_BIN_OP (LHS, LEFT, FMul, RIGHT)
#define CALC_BIN_DIV(LHS, LEFT, RIGHT) LLVM_BIN_OP (LHS, LEFT, FDiv, RIGHT)

#define CALC_UN_MINUS(LHS, ARG) {                                \
    llvm_extra_t * extra = calc_get_extra (scanner);             \
    LHS.rvalue = LLVMBuildFNeg (extra->builder, ARG.rvalue, ""); \
}
#define CALC_UN_PLUS(LHS, ARG) LHS = ARG

#define CALC_X(LHS) {                                \
    llvm_extra_t * extra = calc_get_extra (scanner); \
    LHS.rvalue = extra->x;                           \
  }

#define calc_parse calc_llvm_parse
#include "calc.tab.c"

static void calc_llvm_free (parser_t state)
{
  llvm_state_t * llvm_state = state;
  if (NULL != llvm_state->orc_ref)
    LLVMOrcDisposeInstance (llvm_state->orc_ref);
  free (llvm_state);
}

static uint64_t orc_sym_resolver (const char * name, void * arg)
{
  return (uint64_t)(uintptr_t)LLVMOrcGetSymbolAddress ((LLVMOrcJITStackRef)arg, name);
}


#define ALLOC_FAIL(VAR, TEXT, LABEL)                                \
  if (NULL == VAR)                                                  \
    {                                                               \
      fprintf (stderr, "Failed to allocate memory for " TEXT "\n"); \
      goto LABEL;                                                   \
    }

static parser_t calc_llvm_init (char * expr)
{
  int rv;

  llvm_state_t * llvm_state = malloc (sizeof (*llvm_state));
  ALLOC_FAIL (llvm_state, "parser state", fail);
  llvm_state->orc_ref = NULL;

  llvm_extra_t extra = {
    .ll = { .buf = NULL },
  };

  LLVMModuleRef module = LLVMModuleCreateWithName ("calc_module");
  LLVMTypeRef func_arg_types[] = { LLVM_CALC_TYPE };
  LLVMTypeRef func_type = LLVMFunctionType (
    LLVM_CALC_TYPE,
    func_arg_types, sizeof (func_arg_types) / sizeof (func_arg_types[0]),
    /* IsVarArg = */ 0);
  LLVMValueRef func = LLVMAddFunction (module, "calc", func_type);
  LLVMBasicBlockRef basic_block = LLVMAppendBasicBlock (func, "entry");
  extra.x = LLVMGetParam (func, 0);
  extra.builder = LLVMCreateBuilder ();
  LLVMPositionBuilderAtEnd (extra.builder, basic_block);

  yyscan_t scanner;
  rv = calc_lex_init_extra (&extra, &scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to initialize lexer\n");
      goto dispose_builder;
    }

  calc__scan_string (expr, scanner);
  rv = calc_llvm_parse (scanner);
  calc_lex_destroy (scanner);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to parse\n");
      goto dispose_builder;
    }

  char * error = NULL;
  rv = LLVMVerifyModule (module, LLVMAbortProcessAction, &error);
  if (0 != rv)
    {
      fprintf (stderr, "Failed to verify module: %s\n", error);
      LLVMDisposeMessage(error);
      goto dispose_builder;
    }
  LLVMDisposeMessage(error);
  error = NULL;

  LLVMInitializeNativeTarget ();
  LLVMInitializeNativeAsmPrinter ();
  LLVMLinkInMCJIT ();

  char *def_triple = LLVMGetDefaultTargetTriple ();   // E.g. "x86_64-linux-gnu"
  LLVMTargetRef target_ref;
  if (LLVMGetTargetFromTriple(def_triple, &target_ref, &error))
    {
      fprintf (stderr, "Failed to get target tripple");
      LLVMDisposeMessage (def_triple);
      goto dispose_builder;
    }
   
  if (!LLVMTargetHasJIT(target_ref))
    {
      fprintf (stderr, "Can't JIT here");
      LLVMDisposeMessage (def_triple);
      goto dispose_builder;
    }
   
  LLVMTargetMachineRef tm_ref =
     LLVMCreateTargetMachine (target_ref, def_triple, "", "",
                              LLVMCodeGenLevelDefault,
                              LLVMRelocDefault,
                              LLVMCodeModelJITDefault);
  if (NULL == tm_ref)
    {
      fprintf (stderr, "Failed to create target machine");
      LLVMDisposeMessage (def_triple);
      goto dispose_builder;
    }
  LLVMDisposeMessage (def_triple);

  llvm_state->orc_ref = LLVMOrcCreateInstance (tm_ref);
  LLVMOrcAddEagerlyCompiledIR (llvm_state->orc_ref, module, orc_sym_resolver, llvm_state->orc_ref);

  llvm_state->compiled_func = (void *)LLVMOrcGetSymbolAddress (llvm_state->orc_ref, "calc");

  LLVMDisposeTargetMachine (tm_ref);
  LLVMDisposeBuilder (extra.builder);
  LLVMDisposeModule (module);

  return (llvm_state);

dispose_builder:
  LLVMDisposeBuilder (extra.builder);
  calc_llvm_free (llvm_state);
fail:
  return (NULL);
}

static int calc_llvm_calc (parser_t state, arg_x_t * arg_x, calc_type_t * result)
{
  llvm_state_t * llvm_state = state;
  *result = llvm_state->compiled_func (arg_x->x);
  return (0);
}

parser_funcs_t llvm_parser = {
  .init = calc_llvm_init,
  .calc = calc_llvm_calc,
  .free = calc_llvm_free,
};
