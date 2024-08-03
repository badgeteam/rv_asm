
#include<stdio.h>
#include"comp.h"
#include"main.h"

char*inputBuffer;
size_t inputBufferSize;

void comp(char*input_buffer,size_t input_buffer_size){

  inputBuffer = input_buffer;
  inputBufferSize = input_buffer_size;

  printf("Input Buffer Size = %ld\n",input_buffer_size);
  printf("Input Buffer = \n");
  for(int i = 0;i<input_buffer_size;i++){
    printf("%c",input_buffer[i]);
  }
  
  writeToOutputFile((unsigned char*)"Test",4);
  writeToOutputFile((unsigned char*)"\nTest2",6);

}
