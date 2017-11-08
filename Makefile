YACC=bison
LEX=flex

%.tab.h %.tab.c: %.y
	$(YACC) -d $<

%.lex.h %.lex.c: %.l
	$(LEX) --header-file=$*.lex.h -o $*.lex.c $<

main: main.o calc.lex.o calc.tab.o

calc.h: calc.lex.h calc.tab.h
main.c: calc.h

PHONY: clean

clean:
	$(RM) *.tab.[ch] *.lex.[ch] *.o main