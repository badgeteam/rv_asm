#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>



bool isHexChar(char c);
bool isIdentChar(char c);

void swapBytes(uint8_t*buff, uint64_t index0, uint64_t index1);
uint64_t align(uint64_t n, uint64_t bits);
void exportFile(char*filename,uint8_t*buff,uint64_t size);
uint8_t parseHexChar(char c);
uint64_t parseUIntFromString(char*buff,char*buffTop);
bool StrCmpBegin(char*str,char*buff,char*buffTop);
bool StrCmp(char*str,char*buff,char*buffTop);
char*extractFilename(char*buff,char*buffTop);

void lexError(char*msg,struct File*file, int line);
void compWarning(char*msg, struct Token*token);
void compError(char*msg, struct Token*token);

