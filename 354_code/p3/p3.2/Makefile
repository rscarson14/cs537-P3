
all: random

random: 	random.c libmem.so    
		gcc random.c -lmem -L. -o random -m32

mem.o: 		mem.c
		gcc -c -fpic mem.c -m32

libmem.so: 	mem.o
		gcc -shared -o libmem.so mem.o -m32

clean: 
		rm *.o
		rm *.so
		rm random

