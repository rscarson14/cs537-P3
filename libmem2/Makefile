
all: 		libmem1.so mem1_test.c
		gcc -o mem1_test mem1_test.c -lmem1 -L. -Wall -Werror

libmem1.so:	mem.o
		gcc -shared -o libmem1.so mem.o

mem.o:		mem.c
		gcc -c -fpic mem.c -Wall -Werror

clean:
		rm *.o
		rm *.so
		rm mem1_test
