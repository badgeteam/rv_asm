
CC = gcc
EXE = rvasm

$(EXE): objs/main.o objs/comp.o
	$(CC) -o $(EXE) objs/main.o objs/comp.o

objs/main.o: src/main.c src/main.h
	$(CC) -c -o objs/main.o src/main.c

objs/comp.o: src/comp.c src/comp.h
	$(CC) -c -o objs/comp.o src/comp.c

