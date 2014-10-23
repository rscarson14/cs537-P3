#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include "mem.h"


#define WSIZE	4
#define DSIZE	8


#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) 		(*(unsigned int *)(p))
#define PUT(p, val) 	(*(unsigned int *)(p) = (val))

#define GET_SIZE(p)	(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)	((char *)(bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(BP)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PRE_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static void *fhead = NULL;
static int pol;

static void *first_fit(size_t as){
	void *bp;

	for(bp = fhead; GETSIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
		if(!GET_ALLOC(HDRP(bp)) && (as <= GET_SIZE(HDRP(bp)))){
			return bp;
		}
	}
	return NULL;
}


int Mem_Init(int sizeOfRegion, int policy){
	
	if(fhead != NULL || fhead == -1){
		perror("Mem_Init already called\n");
		return -1;
	}
	
	if(sizeOfRegion <= 0){
		perror("invalid sizeOfRegion\n");
		return -1;
	}

	if(policy < 0 || policy > 2){
		perror("bad policy\n");
		return -1;
	}
	
	pol = policy;



}


void *Mem_Alloc(int size){

}

int Mem_Free(void *ptr){

}

void Mem_Dump(){

}
