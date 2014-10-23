// R scott Carson
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"

#define BLOCK_SIZE 16
#define SHIFTL(i,s) (i << s)
#define SHIFTR(i,s) (i >> s)

void* mem_ptr;
long region_size;
unsigned char* m_bitmap;
unsigned int reserved_bytes;

// Initializes the free chunk of memory to be used during allocation
int Mem_Init(int size){

  long pg_size;
  int mod_res, num_pages, fd_zero;

  if(size <= 0){
    return -1;
  }

  // Align size to page
  pg_size = sysconf(_SC_PAGESIZE);
  num_pages = size/pg_size;
  mod_res = size % pg_size;
  if(mod_res > 0){
    num_pages++;
  }
  region_size = num_pages*pg_size;

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

  /* For workload1 (16 byte allocations, bitmapping will be an effective solution
     that will save a lot of space. I am going to assume memory is broken into 
     16 byte chinks and will have an array of chars where each bit is associated
     with a 16-byte chunk. A 0 signifies that it is not being used and a 1
     signifies that it is being used. This will greatly reduce the complexity of
     the allocation code */

  // First I see how many bytes I will need to satisfy the entire request
  reserved_bytes = region_size / (BLOCK_SIZE * 8);
  
  // Then I recalculate the region_size as if I had max space (e.g. no header)
  region_size = region_size - reserved_bytes;
  
  // Finally I recalulate how many reserved bytes I'll need. This causes some waste,
  // but not too much.
  reserved_bytes = region_size / (BLOCK_SIZE * 8);
  if((region_size % (BLOCK_SIZE * 8)) > 0) { region_size++; }
  
  // I allocate the bitmap array here and increment the main mem_ptr to point to
  // the end of the array.
  printf("*Init* mem_ptr = %p\n", mem_ptr);
  m_bitmap = (unsigned char*) mem_ptr;
  mem_ptr += reserved_bytes;
  printf("*Init* mem_ptr = %p\n", mem_ptr);
  memset(m_bitmap, 0, reserved_bytes);

  return 0;

 error:
  return -1;
}


// Allocates the given size (in bytes) to the user. If the call is unsuccessful,
// 0 will be returned, otherwise a ptr to the allocated chunk of memory.
void* Mem_Alloc(int size){
  
  int i, k;
  unsigned char* tmp_ptr;
  void* ret_ptr = NULL;

  if(size != 16){
    return NULL;
  }

  tmp_ptr = m_bitmap;

  for(i = 0; i < reserved_bytes; i++){
    tmp_ptr += i;
    for(k = 0; k < 8; k++){
      
      if( SHIFTL(0x01,k) & *tmp_ptr ){
	continue;
      }
      else{
	*tmp_ptr = *tmp_ptr | SHIFTL(0x01,k);
	ret_ptr = mem_ptr + (((i * 8) + k) * BLOCK_SIZE);
	return ret_ptr;
      }
    }
  }

  return NULL;
}


// Frees an allocated ptr. Includes error checking.
int Mem_Free(void* ptr){
  
  int i, k;
  unsigned char* tmp_ptr;

  if(ptr == NULL) { return -1; }
  if(ptr > mem_ptr+region_size) { return -1; }
  if(ptr < mem_ptr) { return -1; }
  if((ptr - mem_ptr) % BLOCK_SIZE != 0) { return -1; }

  i = (ptr - mem_ptr) / (BLOCK_SIZE*8);
  k = (ptr - mem_ptr) % (BLOCK_SIZE*8);

  tmp_ptr = m_bitmap + i;
  *tmp_ptr = *tmp_ptr & ~SHIFTL(0x01,k);

  return 0;
}

// Return the number of bytes left that can be allocated (this number won't include overhead)
int Mem_Available(){

  int i, k, counter;
  unsigned char* tmp_ptr;

  // Make sure memory has been initialized
  if(m_bitmap == NULL){
    return -1;
  }
  
  counter = 0;
  tmp_ptr = m_bitmap;

  for(i = 0; i < reserved_bytes; i++){
    tmp_ptr += i;
    for(k = 0; k < 8; k++){
      if( SHIFTL(0x01,k) & *tmp_ptr ){
	continue;
      }
      else{
	counter += 16;
      }
    }
  }

  return counter;
}

// Print out contents of heap
void Mem_Dump(){
  printf("TODO\n");
}


