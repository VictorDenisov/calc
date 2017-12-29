YACC=bison -Wall -Werror
LEX=flex
CFLAGS+=-Wall -Werror -O2 -g -Ilibjit/include
LDLIBS+=-lgccjit libjit/jit/.libs/libjit.a -lm -lpthread

%.tab.h %.tab.c: %.y
	$(YACC) -d $<

%.lex.h %.lex.c: %.l
	$(LEX) --header-file=$*.lex.h -o $*.lex.c $<

main: main.o calc.lex.o compute.o ast.o gccjit.o libjit.o

parser.h: calc.lex.h calc.tab.h
main.o: calc.h parser.h compute.h ast.h gccjit.h libjit.h
compute.o: calc.tab.c calc.h parser.h compute.h
ast.o: calc.tab.c calc.h parser.h ast.h
gccjit.o: calc.tab.c calc.h parser.h gccjit.h
libjit.o: calc.tab.c calc.h parser.h libjit.h
calc.lex.o: calc.h parser.h

PHONY: clean

clean:
	$(RM) *.tab.[ch] *.lex.[ch] *.o main
