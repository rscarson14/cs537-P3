
all: 		libmem3.so libmem2.so libmem1.so

libmem3.so:	mem3.o
		gcc -shared -o libmem3.so mem3.o

libmem2.so:	mem2.o
		gcc -shared -o libmem2.so mem2.o

libmem1.so:	mem1.o
		gcc -shared -o libmem1.so mem1.o

mem3.o:		mem3.c
		gcc -c -fpic mem3.c -Wall -Werror

mem2.o:		mem2.c
		gcc -c -fpic mem2.c -Wall -Werror

mem1.o:		mem1.c
		gcc -c -fpic mem1.c -Wall -Werror

clean:
		rm *.o
		rm *.so
