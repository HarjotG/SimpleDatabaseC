CC=gcc
CFLAGS=-I.
DEPS = hashtable.h siphash.h
OBJ = main.o hashtable.o siphash.o
OBJTEST = test.o hashtable.o siphash.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

db: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

test: $(OBJTEST)
	$(CC) -o $@ $^ $(CFLAGS)