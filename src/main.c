
#include<stdio.h>
#include"main.h"
#include"comp.h"

FILE*fp = NULL;
char*outputfilename = NULL;

int main(int argc, char**argv){

  if(argc != 3){
    fprintf(stderr,"Usage: rvasm <inputfilename> <outputfilename>");
    return -1;
  }
  outputfilename = argv[2];

  fp = fopen(argv[1],"r");
  if(!fp){
    fprintf(stderr,"Unable to open file %s\n",argv[1]);
    return -1;
  }
  fseek(fp,0L,SEEK_END);
  size_t input_buffer_size = ftell(fp);
  fseek(fp,0L,SEEK_SET);
  char*input_buffer = malloc(input_buffer_size);
  fread(input_buffer,1,input_buffer_size,fp);
  fclose(fp);
  fp = NULL;

  comp(input_buffer,input_buffer_size);

  fclose(fp);
  return 0;
}

void writeToOutputFile(unsigned char*buff,size_t size){
  if(!fp){
    fp = fopen(outputfilename,"wb");
    if(!fp){
      fprintf(stderr,"Unable to write to File %s\n",outputfilename);
      exit(-1);
    }
  }
  fwrite(buff,size,1,fp);

}

