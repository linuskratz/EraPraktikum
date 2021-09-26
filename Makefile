CFLAGS=-g -Wall -O2 -msse4.1 

.PHONY: all
all: salsa
salsa: salsa.c salsa20_crypt.c salsa20_core.S 
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f salsa
