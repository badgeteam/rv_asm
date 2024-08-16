#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>



bool isHexChar(char c);
bool isIdentChar(char c);
uint8_t parseHexChar(char c);
char parseChar(char*cp);

bool StrCmpBegin(char*str,char*buff,char*buffTop);
bool StrCmp(char*str,char*buff,char*buffTop);

void compWarning(char*msg, struct Token*token);
void compError(char*msg, struct Token*token);

