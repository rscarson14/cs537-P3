#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

#define SIZE_OF_REGION 4096

int main(int argc, char* argv[]){

  int retval;

  retval = Mem_Init(SIZE_OF_REGION);
  
  if(retval < 0){ printf("Error mapping!\n"); }
  else { printf("map successful!\n"); }

  return 0;
}
