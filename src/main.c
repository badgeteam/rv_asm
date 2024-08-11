
#include<stdio.h>
#include"comp.h"


int main(int argc, char**argv){

  if(argc != 3){
    fprintf(stderr,"Usage: rvasm <inputfilename> <outputfilename>");
    return -1;  
  }

  struct CompContext*ctx = comp(argv[1]);

  return 0;
}



