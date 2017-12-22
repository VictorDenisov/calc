YACC=bison -Wall -Werror
LEX=flex
CFLAGS+=-Wall -Werror -O2 -g

%.tab.h %.tab.c: %.y
	$(YACC) -d $<

%.lex.h %.lex.c: %.l
	$(LEX) --header-file=$*.lex.h -o $*.lex.c $<

main: main.o calc.lex.o compute.o ast.o

parser.h: calc.lex.h calc.tab.h
main.c: calc.h parser.h compute.h ast.h
compute.o: calc.tab.c calc.h parser.h compute.h
ast.o: calc.tab.c calc.h parser.h ast.h
calc.lex.o: calc.h parser.h

PHONY: clean

clean:
	$(RM) *.tab.[ch] *.lex.[ch] *.o main
