CC      = clang
PROG    = test
CFLAGS  = -Wall -g -O2 -I../src
OBJ     = test.c ../src/utf8-validator.c

.PHONY: run

prog: $(OBJ)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ)

clean:
	rm -rf $(OBJS) $(PROG)

run: prog
	./test
