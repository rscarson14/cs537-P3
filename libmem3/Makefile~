
all: 		libmem2.so mem2_test.c
		gcc -o mem2_test mem2_test.c -lmem2 -L. -Wall -Werror

libmem2.so:	mem.o
		gcc -shared -o libmem2.so mem.o

mem.o:		mem.c
		gcc -c -fpic mem.c -Wall -Werror

clean:
		rm *.o
		rm *.so
		rm mem2_test
