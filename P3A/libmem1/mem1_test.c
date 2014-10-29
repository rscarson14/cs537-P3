#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

#define SIZE_OF_REGION 4096
#define ALLOC_SIZE     16

int main(int argc, char* argv[]){

  int retval;

  char *ptr1, *ptr2;

  retval = Mem_Init(SIZE_OF_REGION);
  
  if(retval < 0){ printf("Error mapping!\n"); }
  else { printf("map successful!\n"); }

  printf("free memory = %d\n", Mem_Available());

  Mem_Dump();

  ptr1 = (char*) Mem_Alloc(ALLOC_SIZE);
  if(ptr1 == NULL){ printf("Error allocating!\n"); }
  else{ printf("ptr1 = %p\n", ptr1); }

  Mem_Dump();

  printf("free memory = %d\n", Mem_Available());

  ptr2 = (char*) Mem_Alloc(ALLOC_SIZE);
  if(ptr2 == NULL){ printf("Error allocating!\n"); }
  else{ printf("ptr2 = %p\n", ptr2); }
  
  Mem_Dump();

  printf("free memory = %d\n", Mem_Available());

  retval = Mem_Free(ptr1);
  if(retval < 0){ printf("Error in free!\n"); }
  else{ printf("Free Successful!\n"); }

  printf("free memory = %d\n", Mem_Available());

  Mem_Dump();

  return 0;
}
