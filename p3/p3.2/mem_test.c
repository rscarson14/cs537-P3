#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

int main(int argc, char *argv[]){
	int policy;
	int size;
	int suc;

	if(argc != 3){
		fprintf(stderr, "usage: prog <mem_size> <policy>\n");
		exit(0);
	}

	size = atoi(argv[1]);
	policy = atoi(argv[2]);
	
	if(policy == 0){
		suc = Mem_Init(size, P_BESTFIT);
	}
	else if(policy == 1){
		suc = Mem_Init(size, P_WORSTFIT);
	}
	else if(policy == 2){
		suc = Mem_Init(size, P_FIRSTFIT);
	}
	else{
		fprintf(stderr, "bad policy");
	}

	if(suc != 0){
		fprintf(stderr, "could not initialize memory\n");
		exit(0);
	}

	Mem_Dump();

	void *ptr = Mem_Alloc(8);
	//Mem_Dump();
	//
	printf("*****PTR = %p\n", ptr);
	
	//void *ptr1 = Mem_Alloc(100);

	//Mem_Dump();
	/*
	Mem_Free(ptr1);
	printf("****freed ptr1\n\n");

	Mem_Dump();*/

	//Mem_Free(ptr);
	//Mem_Dump();

	
	//printf("free ptr\n");
	
	//Mem_Free(ptr1);
	//Mem_Dump();

	//printf("free ptr1\n");
	/*
	//Mem_Free(ptr2);
	//printf("****freed ptr2\n\n");

	Mem_Dump();*/

	return 0;
}
