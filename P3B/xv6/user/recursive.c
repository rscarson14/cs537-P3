// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"


void recurse(int n){

  if(n > 0){
    recurse(n-1);
  }
  
  return;
}


int
main(int argc, char *argv[])
{

  recurse(200);

  exit();
}
