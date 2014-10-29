#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mem.h"


typedef int Align; //Used to force 4-byte alignment

union list_header {
	struct { // defines a node in the freelist
		union list_header *next;
		unsigned size;
	} lh;
	Align aln; //used for forced alignment
};
typedef union list_header Header;


struct tnode {
	void *this;
	void *valid;
};
typedef struct tnode T;


static int addTaken(T *ptr);
static int findTaken(T *ptr);


static Header *fhead = NULL; // The head of the list
static Header base; // empty list
static int pol; // chosen allocation policy
static int heapSize;
static T *taken;

int Mem_Init(int sizeOfRegion, int policy){
	
	// Check different error cases
	if(fhead != NULL || fhead ==((void *) -1)){
		fprintf(stderr, "Mem_Init already called\n");
		return -1;
	}
	if(sizeOfRegion <= 0){
		fprintf(stderr, "Bad region\n");
		return -1;
	}
	if(policy != P_BESTFIT && policy != P_WORSTFIT && policy != P_FIRSTFIT){
		fprintf(stderr, "Incorrect policy\n");
		return -1;
	}
	
	pol = policy; // set chosen policy
	int pageSize = getpagesize(); // determine system page size

	int fd = open("/dev/zero", O_RDWR); // open zero'd pages file
	
	sizeOfRegion += sizeof(Header)*129; //include Header overhead in sizeOfRegion
	
	// If the requested region is less than a page, set sizeOfRegion to pageSize
	if(sizeOfRegion < pageSize){
		sizeOfRegion = pageSize;
	}
	// if the requested region is not evenly broken into pages, round up sizeOfRegion
	if((sizeOfRegion % pageSize) != 0){
		sizeOfRegion = ((sizeOfRegion/pageSize) + 1) * pageSize;
	}
	 // request memory
	fhead = mmap(NULL, (sizeOfRegion), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	
	taken =(void *)fhead;
	fhead += 128;
	
	heapSize = sizeOfRegion;
	
	base.lh.size = 0;
	base.lh.next = fhead;
	fhead->lh.size = sizeOfRegion;
	fhead->lh.next = fhead;
	
	if(fhead == (Header *) MAP_FAILED){
		fprintf(stderr, "mmap\n");
		return -1;
	}

	close(fd);
	return 0;

}






void *Mem_Alloc(int size){
		
	if((size <= 0) || (fhead == NULL)){
		return NULL;
	}

	size += sizeof(Header);
	
	Header *blkPtr, *prev;

	if(pol == P_BESTFIT){
		
		Header *best = NULL;
		int fit = -1;

		blkPtr = prev = fhead;
		for(blkPtr = prev->lh.next; ; prev = blkPtr, blkPtr = blkPtr->lh.next){
			if(blkPtr->lh.size >= size){
				if(blkPtr->lh.size == size){
					prev->lh.next = blkPtr->lh.next;
				}
				else {
					if((blkPtr->lh.size - size) < fit){
						best = blkPtr;
						fit = blkPtr->lh.size - size;
					}
				}
			}
			if(blkPtr == fhead){
				break;
			}
		}
		if(best != NULL && fit != -1){
			best->lh.size -= size;
			best = (Header *)(((char *) best) + best->lh.size);
			best->lh.size = size;

			return (void *)(best + 1);
		}
		else{
			return NULL;
		}
	}
        else if(pol == P_WORSTFIT){
		
		//printf("in worsfit alloc\n");

		Header *worst = NULL;
		int fit = -1;

                blkPtr = prev = fhead;
                for(blkPtr = prev->lh.next; ; prev = blkPtr, blkPtr = blkPtr->lh.next){
                        if(blkPtr->lh.size >= size){
                                if(blkPtr->lh.size == size){
					prev->lh.next = blkPtr->lh.next;
				}
				else {
					if(((blkPtr->lh.size - size) < fit)&&((blkPtr->lh.size) - size >= 0)){
					        worst = blkPtr;
					        fit = blkPtr->lh.size - size;
					}
				}
			}
			if(blkPtr == fhead){
				break;
			}
		}
		if(worst != NULL && fit != -1){
			worst->lh.size -= size;
			worst = (Header *)(((char *) worst) + worst->lh.size);
			worst->lh.size = size;
			
			return (void *)(worst + 1);
		}
		else{
			return NULL;
		}
	}
	else if(pol == P_FIRSTFIT){
		
		blkPtr = prev = fhead;
  
	        for(blkPtr = prev->lh.next; ; prev = blkPtr, blkPtr = blkPtr->lh.next){
		       if(blkPtr->lh.size >= size){
		                if(blkPtr->lh.size == size){
		                     prev->lh.next = blkPtr->lh.next;
				 }
				 else {
		                        blkPtr->lh.size -= size;		
					blkPtr =(Header *)(((char *) blkPtr) + blkPtr->lh.size);
					blkPtr->lh.size = size;
			       }
			        fhead = prev;
				return (void *)(blkPtr + 1);
		        }
			if(blkPtr == fhead){
				return NULL;
			}
	        }
	
	}
}


int Mem_Free(void *ptr){
	Header *blkPtr, *p;
	
	//printf("free fhead = %p\n", fhead);

	if(ptr == NULL){
		return -1;
	}
	 
	if((((Header *) ptr) > (fhead) + heapSize) ||((Header *) ptr < fhead)){
		return -1;
	}
	
	//printf("*free fhead = %p\n", fhead);

	blkPtr = (Header *) ptr - 1;
	for(p = fhead; !(blkPtr > p && blkPtr < p->lh.next); p = p->lh.next){
		
		//printf("for loop fhead = %p\n", fhead);

		if(p >= p->lh.next && (blkPtr > p || blkPtr < p->lh.next)){
			/*
			printf("free arena found\n");

			printf("p = %p\n", p);
			printf("blkPtr = %p\n", blkPtr);
			printf("p next = %p\n", p->lh.next);
			printf("blkPtr next = %p\n", blkPtr->lh.next);
			*/
			break;
		}
	}
	
        //printf("**free fhead = %p\n", fhead);

	if((((char *)blkPtr) + blkPtr->lh.size) ==((char *)p->lh.next)){
		blkPtr->lh.size += p->lh.next->lh.size;
		//printf("***free fhead = %p\n", fhead);
		blkPtr->lh.next = p->lh.next->lh.next;
		//printf("***free fhead = %p\n", fhead);

	}
	else{
		//printf("1free p = %p\n", p);
		//printf("1free fhead = %p\n", fhead);
		blkPtr->lh.next = p->lh.next;
	}

	if((((char *)p) + p->lh.size) ==((char *)blkPtr)){
		p->lh.size += blkPtr->lh.size;
		p->lh.next = blkPtr->lh.next;
	}
	else{
		//printf("2free p = %p\n", p);
		//printf("2free fhead = %p\n", fhead);
		p->lh.next = blkPtr;
	}

 	//printf("end free p = %p\n", p); 
	//printf("end free fhead = %p\n", fhead);

	fhead = p;
}



void Mem_Dump(){
	
	Header *blkPtr = fhead;
	//printf("dump fhead : %p\n", fhead);
	if(fhead == NULL){
	//	printf("empty list\n");
	}

	while(blkPtr->lh.next != fhead){
	//	 printf("loop size: %i , ", blkPtr->lh.size);
	//	 printf("loop next: %p\n", blkPtr->lh.next);
		 blkPtr = blkPtr->lh.next;
	}

	if(blkPtr->lh.next == fhead || fhead->lh.next == fhead){
		//printf("last size: %i , ", blkPtr->lh.size);
		//printf("last next: %p\n", blkPtr->lh.next);
	}
	// printf("2dump fhead : %p\n", fhead);
}



static int addTaken(T *ptr){
	
	if(ptr == NULL){
		return -1;
	}

	if(taken == NULL){
		taken->this = ptr;
		taken->valid = 1;
		return 0;
	}

	
	//while(
		
}
