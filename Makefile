
CC = gcc
EXE = rvasm

$(EXE): objs/main.o objs/comp.o objs/token.o objs/util.o objs/export.o
	$(CC) -o $(EXE) objs/main.o objs/comp.o objs/token.o objs/util.o objs/export.o

objs/main.o: src/main.c
	$(CC) -c -o objs/main.o src/main.c

objs/comp.o: src/comp.c src/comp.h
	$(CC) -c -o objs/comp.o src/comp.c

objs/token.o: src/token.c src/token.h
	$(CC) -c -o objs/token.o src/token.c

objs/util.o: src/util.c src/util.h
	$(CC) -c -o objs/util.o src/util.c

objs/export.o: src/export.c src/export.h
	$(CC) -c -o objs/export.o src/export.c

