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

define GEN_RULE
%.${1}.o: %.c
	$(COMPILE.c) -Dcalc_type_t="$(subst _, ,${1})" -o $$@ $$<

main.${1}: main.${1}.o calc.lex.${1}.o compute.${1}.o ast.${1}.o cast_expr.${1}.o gccjit.${1}.o libjit.${1}.o llvm.${1}.o elf.${1}.o dl.${1}.o
parser.h: calc.tab.h calc.lex.h
main.${1}.o: calc.h parser.h compute.h ast.h gccjit.h libjit.h llvm.h
compute.${1}.o: calc.tab.c calc.h parser.h compute.h
ast.${1}.o: calc.tab.c calc.h parser.h ast.h
gccjit.${1}.o: calc.tab.c calc.h parser.h gccjit.h
libjit.${1}.o: calc.tab.c calc.h parser.h libjit.h
llvm.${1}.o: calc.tab.c calc.h parser.h llvm.h
elf.${1}.o: calc.h elf.h cast_expr.h
dl.${1}.o: calc.h dl.h cast_expr.h
calc.lex.${1}.o: calc.h parser.h
endef

$(foreach _type,$(TYPES),$(eval $(call GEN_RULE,$(_type))))

.PHONY: test
test: ${MAINS}
	py.test -v test.py
