all: myinit

#CFLAGS=-c -W -Wall -O3 
CFLAGS=-c -W -Wall -g3 

myinit: main.o
	gcc -o myinit main.o -lm

main.o: main.c
	gcc ${CFLAGS} main.c

.PHONY: all clean

clean:
	rm -f *.o myinit
