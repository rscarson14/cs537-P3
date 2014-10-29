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
#define TEST_OFFSET  16

#define GET(p)          (*(unsigned long *)(p))
#define PUT(p, value)   (*(unsigned long *)(p) = (value))

#define GET_ALLOC(p)    (GET(p) & 0xF)
#define GET_SIZE(p)     (GET(p) >> 4)

#define HDR_S_A(p)      ((unsigned long *) (((void*) (p)) - 8))
#define HDR_TEST(p)     ((unsigned long *) (((void*) (p)) - TEST_OFFSET))
#define FTR_S_A(p)      ((unsigned long *) (((void*) (p)) + GET_SIZE(HDR_S_A(p))))  

#define COMB_S_A(s,a)   (((s) << 4) | (a))

// Gets the next pointer by adding the size of the current chunk, the current FTR, and the next HDR
#define NEXT_B(p)         ((void *)(p) + (GET_SIZE(HDR_S_A(p)) + FTR_SIZE + HDR_SIZE))
// Gets the previous pointer by subtracting the size of the current header and previous footer, and 
// the previous block size
#define PREV_B(p)         ((void *)(p) - (GET_SIZE(((void*) (p) - HDR_SIZE - FTR_SIZE))))


/* Header Structure:
   long TEST = DEADBEEF000DECAF
   long Size and Alloc = bits 63-4 are for size, bits 3-0 are for allocation

   Footer Structure:
   long Size and Alloc = bits 63-4 are for size, bits 3-0 are for allocation
 */


void* mem_ptr; // stores the beginning of our entire memory region
void* epi_ptr; // store the end of our entire memory region

void* fhead; // points to the first valid blk in memory (used at the start of first fit)

int available_mem;
long region_size;

int Mem_Init(int size){
  
  long pg_size;
  int mod_res, num_pages, fd_zero;

  // Mem_init already called
  if(mem_ptr > (void*) 0){
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
  //  printf("Intitial region size = %ld\n", region_size);
  
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

  /* For workload 2 I am implementing a first fit allocator that can Free and Coalesce in 
     constant time. Each block of memory has a 16-byte header, where the first 8-bytes is 
     a test number to ensure I allocated the chunk, and the second 8-bytes is a combination
     of the size and whether or not the block is allocated. Each Block also has an 8-byte
     footer that contains the size and allocation bit as well. The final 8-bytes in the 
     requested memory region is an epilogue footer that contains a size of 0 and an
     an allocation bit as one. This makes it easy to handle edge cases. */

  //Initialize the first header and prologue
  fhead = mem_ptr;
  epi_ptr = mem_ptr + region_size;
  //printf("epi_ptr = %p\n", epi_ptr);

  PUT(HDR_S_A(epi_ptr), COMB_S_A(0,1)); // 8 bytes
  PUT(HDR_TEST(epi_ptr), TEST_VAL);
  PUT(fhead, COMB_S_A(8,1)); // 8 bytes
  fhead += 8;
  PUT(fhead, COMB_S_A(8,1)); // 8 bytes
  fhead += 8;
  
  fhead = fhead + HDR_SIZE;
  available_mem = region_size - 3*HDR_SIZE - FTR_SIZE;
  PUT(HDR_TEST(fhead), TEST_VAL);
  PUT(HDR_S_A(fhead), COMB_S_A(available_mem, 0));

  return 0;

 error:
  return -1;
}


void insertBlock(void *bp, int size){
  
  int chunk_size;
  
  //  printf("Inserting block\n");
  chunk_size = GET_SIZE(HDR_S_A(bp));
  //  printf("chunk_size = %d", chunk_size);

  // Only split the chunk if there will be enough for another block (at least 16 bytes +
  // hdr and ftr.
  if((chunk_size - size) >= (SIZE_16 + FTR_SIZE + HDR_SIZE)){
    // printf("Splitting\n");
    PUT(HDR_TEST(bp), TEST_VAL);
    PUT(HDR_S_A(bp), COMB_S_A(size - HDR_SIZE - FTR_SIZE, 1));
    PUT(FTR_S_A(bp), COMB_S_A(size - HDR_SIZE - FTR_SIZE, 1));

    //printf("created first header: size = %ld\n", GET_SIZE(HDR_S_A(bp)));
    bp = NEXT_B(bp);
    //    printf("moved ptr to new block: adr = %p size = %d\n", bp, chunk_size - size);
    PUT(HDR_S_A(bp), COMB_S_A((chunk_size - size), 0));
    PUT(FTR_S_A(bp), COMB_S_A((chunk_size - size), 0));
    PUT(HDR_TEST(bp), TEST_VAL);
  }
  // Otherwise just allocate the whole block
  else{
    PUT(HDR_TEST(bp), TEST_VAL);
    PUT(HDR_S_A(bp), COMB_S_A(size, 1));
    PUT(FTR_S_A(bp), COMB_S_A(size, 1));
  }
  
  //  printf("block inserted\n");
}


void* Mem_Alloc(int size){

  void *bp;
  int adjusted_size;

  if( size <= 0 || (size != SIZE_16 && size != SIZE_80 && size != SIZE_256)){
    perror("invalid allocation size\n");
    goto error;
  }

  if(fhead == NULL){
    perror("Memory not initialized");
    goto error;
  }

  //printf("Mem_Alloc: valid size\n");
  // Adjust size to fit the Header and Footer
  adjusted_size = size + HDR_SIZE + FTR_SIZE;

  for(bp = fhead; (GET_SIZE(HDR_S_A(bp))) > 0; bp = NEXT_B(bp)){
    if(!(GET_ALLOC(HDR_S_A(bp))) && (GET_SIZE(HDR_S_A(bp)) >= adjusted_size)){
      insertBlock(bp, adjusted_size);
      available_mem -= adjusted_size;
      return bp;
    }
  }  

 error:
  return NULL;
}


void* coalesce(void* ptr){

  int prev_alloc, next_alloc, size;

  prev_alloc = GET_ALLOC(HDR_S_A(PREV_B(ptr)));
  next_alloc = GET_ALLOC(HDR_S_A(NEXT_B(ptr)));
  size = GET_SIZE(HDR_S_A(ptr)) + HDR_SIZE + FTR_SIZE;

  if(prev_alloc && next_alloc){
    //printf("no coalesce\n");
    available_mem += (HDR_SIZE + FTR_SIZE);
  }
  else if(!prev_alloc && next_alloc){
    //printf("coalesce up\n");
    size += GET_SIZE(HDR_S_A(PREV_B(ptr)));
    PUT(FTR_S_A(ptr), COMB_S_A(size, 0));
    PUT(HDR_S_A(PREV_B(ptr)), COMB_S_A(size, 0));
    ptr = PREV_B(ptr);
    available_mem += (HDR_SIZE + FTR_SIZE);
  }
  else if(prev_alloc && !next_alloc){
    //printf("coalesce down\n");
    size += GET_SIZE(HDR_S_A(NEXT_B(ptr)));
    PUT(HDR_S_A(ptr), COMB_S_A(size, 0));
    PUT(FTR_S_A(ptr), COMB_S_A(size, 0));
    available_mem += (HDR_SIZE + FTR_SIZE);
  }
  else{
    //printf("coalesce both\n");
    size += (GET_SIZE(HDR_S_A(PREV_B(ptr))) + GET_SIZE(HDR_S_A(NEXT_B(ptr))));
    PUT(HDR_S_A(PREV_B(ptr)), COMB_S_A(size, 0));
    PUT(FTR_S_A(NEXT_B(ptr)), COMB_S_A(size, 0));
    ptr = PREV_B(ptr);
    available_mem += (2*HDR_SIZE + 2*FTR_SIZE);
  }

  return NULL;
}


int Mem_Free(void* ptr){
  
  int size;

  if(ptr == NULL || (ptr < mem_ptr) || (ptr > epi_ptr)){
    perror("invalid ptr\n");
    goto error;
  }

  if(*(HDR_TEST(ptr)) != TEST_VAL){
    perror("ptr test val failed\n");
    goto error;
  }

  if(!GET_ALLOC(HDR_S_A(ptr))){
    perror("ptr not allocated\n");
    goto error;
  }

  size = GET_SIZE(HDR_S_A(ptr));

  PUT(HDR_S_A(ptr), COMB_S_A(size, 0));
  PUT(FTR_S_A(ptr), COMB_S_A(size, 0));

  ptr = coalesce(ptr);

  available_mem += size;

  return 0;

 error:
  return -1;
}


int Mem_Available(){
  
  return available_mem;
}


void Mem_Dump(){
  
  void *bp;

  //printf("in mem_dump\n");
  if(fhead == NULL){
    perror("Memory not initialized");
    return;
  }

  for(bp = fhead; (GET_SIZE(HDR_S_A(bp))) > 0; bp = NEXT_B(bp)){
    // printf("addr: %p, size: %ld\n", bp, GET_SIZE(HDR_S_A(bp)));
    if(!(GET_ALLOC(HDR_S_A(bp)))){
      printf("Addr: %p, size: %ld\n", bp, GET_SIZE(HDR_S_A(bp)));
    }
  }
  //printf("mem_dump() end\n");

  return;
}
