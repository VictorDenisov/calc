TYPES := long_double double float long_int int char
MAINS := $(addprefix main.,${TYPES})
CALC_TYPE_T := long_double

YACC=bison -Wall -Werror
LEX=flex
CFLAGS+=-Wall -Werror -O2 -g -Ilibjit/include -I/usr/lib/llvm-3.9/include
LDLIBS+=-lpthread -lrt -ldl -ltinfo -lpthread -lz -lm

%.tab.h %.tab.c: %.y
	$(YACC) -d $<

%.lex.h %.lex.c: %.l
	$(LEX) --header-file=$*.lex.h -o $*.lex.c $<

main: main.${CALC_TYPE_T}
	cp "$<" "$@"

.PHONY: clean
clean:
	$(RM) -v *.tab.[ch] *.lex.[ch] *.o main ${MAINS}

${MAINS}: main.%: main.%.o calc.lex.%.o compute.%.o ast.%.o cast_expr.%.o gccjit.%.o libjit.%.o llvm.%.o elf.%.o dl.%.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
parser.h: calc.tab.h calc.lex.h
main.%.o: main.c calc.h parser.h compute.h ast.h gccjit.h libjit.h llvm.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
compute.%.o: compute.c calc.tab.c calc.h parser.h compute.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
ast.%.o: ast.c calc.tab.c calc.h parser.h ast.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
gccjit.%.o: gccjit.c calc.tab.c calc.h parser.h gccjit.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
libjit.%.o: libjit.c calc.tab.c calc.h parser.h libjit.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
llvm.%.o: llvm.c calc.tab.c calc.h parser.h llvm.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
elf.%.o: elf.c calc.h elf.h cast_expr.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
dl.%.o: dl.c calc.h dl.h cast_expr.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
calc.lex.%.o: calc.lex.c calc.h parser.h
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<
cast_expr.%.o: cast_expr.c
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,$*)" $(OUTPUT_OPTION) $<

.PHONY: test
test: ${MAINS}
	py.test -v test.py
