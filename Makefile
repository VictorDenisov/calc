YACC=bison -Wall -Werror
LEX=flex
CFLAGS+=-Wall -Werror -O2 -g

%.tab.h %.tab.c: %.y
	$(YACC) -d $<

%.lex.h %.lex.c: %.l
	$(LEX) --header-file=$*.lex.h -o $*.lex.c $<

main: main.o calc.lex.o calc.compute.o calc.ast.o

calc.parser.h: calc.lex.h calc.tab.h
main.c: calc.h calc.parser.h calc.compute.h calc.ast.h
calc.compute.h: yyscan.h
calc.compute.o: calc.tab.c calc.h calc.parser.h calc.compute.h
calc.ast.h: yyscan.h
calc.ast.o: calc.tab.c calc.h calc.parser.h calc.ast.h
calc.lex.o: calc.h calc.parser.h

PHONY: clean

clean:
	$(RM) *.tab.[ch] *.lex.[ch] *.o main
