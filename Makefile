all: chinadns

chinadns: src/chinadns.c
	gcc -Wall -O3 -lresolv -o chinadns src/chinadns.c

.PHONY: clean all

clean:
	rm -f chinadns


