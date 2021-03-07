CC=gcc
LEX=lex
BISON=bison
STD=-std=c99
WARN=-Wall -Wextra -Wpedantic -Wformat-security -Wfloat-equal -Wshadow\
     -Wconversion -Winline #-Wpadded
OPT=-O2 -march=native -ffinite-math-only -fno-signed-zeros -DLOG_LEVEL=LOG_LEVEL_WARNING
DBG=-O0 -g -ggdb -DLOG_LEVEL=LOG_LEVEL_DEBUG -DTERM_COLORS
EXTRA=-I. -D_POSIX_C_SOURCE=200809L
LINK=#-lfl
FLAGS=$(STD) $(WARN) $(OPT) $(EXTRA) $(LINK)

all: e6

test: e6
	./etapa6 ../sample.txt
	./etapa6 ../e2_test

e6: scanner parser
	$(CC) lex.yy.c parser.tab.c hash.c ast.c main.c semantic.c tac.c asm.c $(FLAGS) -o etapa6

scanner:
	$(LEX) scanner.l

parser:
	$(BISON) -v --defines="parser.tab.h" parser.y

clean:
	rm -f lex.yy.c parser.tab.h parser.tab.c parser.output etapa6
