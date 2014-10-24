/* R Scott Carson */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"


#define SIZE_256 256
#define SIZE_80  80
#define SIZE_16  16
#define HDR_SIZE 16
#define FTR_SIZE 8

#define TEST_VAL     0xDEADBEEF000DECAF

#define GET(p)          (*(unsigned long *) (p))
#define PUT(p, value)   (*(unsigned long *) (p) = value)

#define HDR_S_A(p)      ((unsigned long *) (((char*) (p)) - 8))
#define HDR_TEST(p)     ((unsigned long *) (((char*) (p)) - 16)) 

#define COMB_S_A(s,a)   ((long)((s << 4) | (a)))

#define GET_ALLOC(p)    (GET(p) & 0xF)
#define GET_SIZE(p)     (GET(p) >> 4)

// Gets the next pointer by adding the size of the current chunk, the current FTR, and the next HDR
#define NEXT(p)         ((void *)(p) + (GET_SIZE(p) + FTR_SIZE + HDR_SIZE))
#define PREV(p)         ((void *)(p) + (GET_SIZE(((void*) (p)) - HDR_SIZE - FTR_SIZE))) 


/* Header Structure:
   long TEST = DEADBEEF000DECAF
   long Size and Alloc = bits 63-4 are for size, bits 3-0 are for allocation

   Footer Structure:
   long Size and Alloc = bits 63-4 are for size, bits 3-0 are for allocation
 */


void* mem_ptr; // stores the beginning of our entire memory region
void* epi_ptr; // store the end of our entire memory region

void* init_ptr; // points to the first valid blk in memory (used at the start of first fit)

int available_mem;
long region_size;

int Mem_Init(int size){
  
  long pg_size;
  int mod_res, num_pages, fd_zero;

  // Mem_init already called
  if(fhead > (void*) 0){
    perror("Mem_Init already called\n");
    goto error;
  }

  if(size <= 0){
    perror("Invalid size\n");
    goto error;
  }

  // Align size to page
  pg_size = sysconf(_SC_PAGESIZE);
  num_pages = size/pg_size;
  mod_res = size % pg_size;
  if(mod_res > 0){
    num_pages++;
  }
  region_size = num_pages*pg_size;
  printf("Intitial region size = %ld\n", region_size);
  
  // Map the required amount of memory
  fd_zero = open("/dev/zero", O_RDWR);
  if(fd_zero < 0){
    goto error;
  }

  mem_ptr = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_zero, 0);
  if(mem_ptr == MAP_FAILED){
    perror("Mmap failed\n");
    exit(1);
  }

  fd_zero = close(fd_zero);
  if(fd_zero < 0){
    goto error;
  }

  /* For workload 2 I am implemeneting a first fit allocator that can Free and Coalesce in 
     constant time. Each block of memory has a 16-byte header, where the first 8-bytes is 
     a test number to ensure I allocated the chunk, and the second 8-bytes is a combination
     of the size and whether or not the block is allocated. Each Block also has an 8-byte
     footer that contains the size and allocation bit as well. The final 8-bytes in the 
     requested memory region is an epilogue footer that contains a size of 0 and an
     an allocation bit as one. This makes it easy to handle edge cases. */

  //Initialize the first header and prologue
  init_ptr = mem_ptr;
  epi_ptr = mem_ptr + region_size;

  PUT(epi_ptr, COMB_S_A(0,1))
  PUT(init_ptr, COMB_S_A(0,1));
  init_ptr += 8;
  PUT(init_ptr, COMB_S_A(0,1));
  init_ptr += 8;
  
  init_ptr = init_ptr_ptr + HDR_SIZE;
  mem_available = region_size - HDR_SIZE - 2*FTR_SIZE;
  PUT(HDR_TEST(init_ptr), TEST_VAL);
  PUT(HDR_S_A(init_ptr), COMB_S_A(mem_available, 0));

  return 0;

 error:
  return -1;
}

void* Mem_Alloc(int size){

  return NULL;
}

int Mem_Free(void* ptr){

  return 0;
}

int Mem_Available(){

  return 0;
}

void Mem_Dump(){

  return;
}
