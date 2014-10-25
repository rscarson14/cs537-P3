
all: 		libmem3.so mem3_test.c
		gcc -o mem3_test mem3_test.c -lmem3 -L. -Wall -Werror

libmem3.so:	mem.o
		gcc -shared -o libmem3.so mem.o

mem.o:		mem.c
		gcc -c -fpic mem.c -Wall -Werror

clean:
		rm *.o
		rm *.so
		rm mem3_test
