
all: test_prog

test_prog: 	mem_test.c libmem.so
		gcc mem_test.c -lmem -L. -o myprogram -m32

mem1.o: 		mem1.c
		gcc -c -fpic mem1.c -m32

libmem.so: 	mem1.o
		gcc -shared -o libmem.so mem1.o -m32

clean: 
		rm *.o
		rm *.so
		rm myprogram
