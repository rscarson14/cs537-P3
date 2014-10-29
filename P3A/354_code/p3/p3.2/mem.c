///////////////////////////////////////////////////////////////////////////////
//
//Author: R. Scott Carson
//cs 354-3 Project 3
//Files: mem.c, mem.h, README, Makefile
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include "mem.h"

#define WSIZE	4
#define DSIZE	8

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) 		(*(unsigned int *)(p))
#define PUT(p, val) 	(*(unsigned int *)(p) = (val))

#define GET_SIZE(p)	(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)	((char *)(bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static void *fhead = NULL; // points to head of free list
static void *epi = NULL; // tracks position of epilogue
static int pol; // keeps track of policy
static int size;

// implementation of bestfit policy
static void *best_fit(size_t as){
	void *bp;
	void *best = NULL;
	size_t fit = (size_t) -1;

	for(bp = fhead; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
			
		if(!GET_ALLOC(HDRP(bp)) && (as <= GET_SIZE(HDRP(bp)))){
	
			if(((GET_SIZE(HDRP(bp))) - as) <= fit){
				best = bp;
				fit = (GET_SIZE(HDRP(bp))) - as;
			}
		}
	}

	return best;
}


//implementation of worstfit policy
static void *worst_fit(size_t as){
	void *bp;
	void *worst = NULL;
	size_t fit = 0;

	for(bp = fhead; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
	
		if(!GET_ALLOC(HDRP(bp)) && (as <= GET_SIZE(HDRP(bp)))){
		
			if(((GET_SIZE(HDRP(bp))) - as) >= fit){
				worst = bp;
				fit = (GET_SIZE(HDRP(bp))) - as;
			}
		}
	}

	return worst;
}


//implementation of firstfit policy
static void *first_fit(size_t as){
	void *bp;

	for(bp = fhead; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
	
		if(!GET_ALLOC(HDRP(bp)) && (as <= GET_SIZE(HDRP(bp)))){
			return bp;
		}
	}

	return NULL;
}


// checks the given ptr to see if it has been allocated by Mem_Alloc
static int checkPtr(void *ptr){
	void *bp;

	for(bp = fhead; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
		
		if((ptr == bp) && (GET_ALLOC(HDRP(bp)))){
			return 0;
		}
	}
	return -1;
}


// Given a pointer to a block, all possible coalesce scenarios are checked
static void *coalesce(void *bp){
	
	//printf("coalesce\n");

	if(bp == NULL){
		return bp;
	}
	
	//printf("fhead = %p\n", fhead);
	//printf("epi = %p\n", epi);
	//printf("size = %p\n", size);

	//printf("pa\n");
	//printf("bp = %p\n", bp);
	//printf("PREV bp = %p\n", PREV_BLKP(bp));
	//printf("HEADERPREV bp = %p\n", HDRP(PREV_BLKP(bp)));
	//printf("ALLOC PREV bp = %p\n", GET_ALLOC(HDRP(PREV_BLKP(bp))));
	
	size_t pa = GET_ALLOC(HDRP(PREV_BLKP(bp)));
	//printf("na\n");
	size_t na = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	//printf("bp size\n");
	size_t size = GET_SIZE(HDRP(bp));

	if(pa && na){
	//	printf("no coalesce\n");
		return bp;
	}
	else if(!pa && na){
	//	printf("coalesce up\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size , 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		return bp;
	}
	else if(pa && !na){
	//	printf("coalesce down\n");
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		return bp;
	}
	else{
	//	printf("both coalesce\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		return bp;
	}
}


// Insert a given size block into a spot pointed to by bp. It splits if the remaining
// block is at least 16 bytes
static void insert(void *bp, size_t as){
		
	size_t cs = GET_SIZE(HDRP(bp));

	if((cs-as) >= (2*DSIZE)){
		//printf("splitting\n");
		PUT(HDRP(bp), PACK(as, 1));
		PUT(FTRP(bp), PACK(as, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(cs - as, 0));
		PUT(FTRP(bp), PACK(cs - as, 0));
		//printf("done splitting\n");
	}
	else{
		PUT(HDRP(bp), PACK(cs, 1));
		PUT(FTRP(bp), PACK(cs, 1));
	}
}


// Initialize memory to given size.  This function rounds up to the nearest page size.
// Memory may only be initialized once.
int Mem_Init(int sizeOfRegion, int policy){
	
	if(fhead != NULL || fhead == (void *) -1){
		perror("Mem_Init already called\n");
		return -1;
	}
	if(sizeOfRegion <= 0){
		perror("invalid sizeOfRegion\n");
		return -1;
	}
	if(policy != P_BESTFIT && policy != P_WORSTFIT && policy != P_FIRSTFIT){
		perror("bad policy\n");
		return -1;
	}
	
	pol = policy;
	int pageSize = getpagesize();
	
	if(sizeOfRegion < pageSize){
		sizeOfRegion = pageSize;
	}
	if((sizeOfRegion % pageSize) != 0){
		sizeOfRegion = ((sizeOfRegion/pageSize) + 1) * pageSize;
	}
	
	int fd = open("/dev/zero", O_RDWR);
	fhead = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	
	PUT(fhead, PACK(DSIZE, 1));
	PUT(fhead + WSIZE, PACK(DSIZE, 1));
	PUT(fhead + ((size_t) sizeOfRegion), PACK(0,1));
	
	size = sizeOfRegion;
	//printf("region = %p\n", sizeOfRegion);
	//printf("fhead = %p\n", fhead);
	//printf("*****EPILOGUE***** = %p\n", fhead + ((size_t) sizeOfRegion));
	epi = fhead + ((size_t) sizeOfRegion);

	if(fhead == MAP_FAILED){
		perror("mmap");
		return -1;
	}

	close(fd);
	
	fhead += WSIZE;
	PUT(fhead + (1*WSIZE), PACK((sizeOfRegion - (6*WSIZE)), 0));
	PUT(fhead + ((2*WSIZE) + (sizeOfRegion - (6*WSIZE))), PACK((sizeOfRegion - (6*WSIZE)), 0));

	return 0;
}


// Allocates memory in 4-byte aligned chunks depending on the chosen policy
void *Mem_Alloc(int size){
	
	//printf("alloc\n");
	
	size_t as;
	void *bp;

	if(fhead == NULL || fhead ==(void *) -1){
		return NULL;
	}
	
	if(size <= 0){
		return NULL;
	}

	if(size <= WSIZE){
		size = WSIZE + DSIZE;
	}
	else{
		size += DSIZE;
	}
	
	if(size % 4 != 0){
		size = ((size/WSIZE) + 1) * WSIZE;
	}

	//printf("size = %i\n", size);
	as = size;

	if(pol == P_BESTFIT){
		bp = best_fit(as);
	}
	else if(pol == P_WORSTFIT){
		bp = worst_fit(as);
	}
	else if(pol == P_FIRSTFIT){
		bp = first_fit(as);
	}
	else
		perror("malloc error)");

	if(bp != NULL){
		insert(bp, size);
		return bp;
	}
	
}


// Checks to make sure the pointer was allocated by Mem_Alloc, then frees
// and coalesces the given block in memory
int Mem_Free(void *ptr){
	
	//printf("free\n");

	if(ptr == NULL){
		return -1;
	}

	if(GET_ALLOC(HDRP(ptr)) == 0){
		return -1;
	}

	if(checkPtr(ptr) == -1){
		return -1;
	}
	
	size_t size = GET_SIZE(HDRP(ptr));
	
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	//printf("free->coel\n");
	
	if(coalesce(ptr) != NULL){
		//printf("end free\n");
		return 0;
	}
	else
		return -1;
}


// Helper function that prints the contents of memory to the screen
void Mem_Dump(){
	
	void *bp;

	if(fhead == NULL){
		perror("mem not initialized");
	}

	for(bp = fhead; (GET_SIZE(HDRP(bp))) > 0; bp = NEXT_BLKP(bp)){
		if(!(GET_ALLOC(HDRP(bp)))){
			printf("Pointer: %p, size: %i\n", bp, GET_SIZE(HDRP(bp)));
		}
	}
}
