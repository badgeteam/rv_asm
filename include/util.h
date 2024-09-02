#pragma once

#include<stdint.h>
#include<stdbool.h>

uint32_t align(uint32_t n, uint32_t b);

bool isHexChar(char c);
bool isIdentChar(char c);
uint8_t parseHexChar(char c);
char parseChar(char*cp);

bool StrCmpBegin(char*str,char*buff,char*buffTop);
bool StrCmp(char*str,char*buff,char*buffTop);

