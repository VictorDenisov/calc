YACC=bison -y
LEX=flex

calc: calc.lex.c calc.lex.h calc.tab.h calc.tab.c
	$(CC) $^ -o $@

calc.lex.c calc.lex.h: calc.l
	$(LEX) --header-file=calc.lex.h -o $@ $<

calc.tab.c: calc.y
	$(YACC) $^

calc.tab.h: calc.y
	$(YACC) -d $^
