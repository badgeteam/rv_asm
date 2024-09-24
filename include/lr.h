#pragma once
#include"common_types.h"

// Does advance after the last token
bool lrParseExpression(CompContext*ctx);

uint32_t getUInt(CompContext*ctx);
int32_t getInt(CompContext*ctx);
uint32_t getUImm(CompContext*ctx, uint32_t bits);
uint32_t getImm(CompContext*ctx, uint32_t bits);

void printLrToken(LrToken*lr);
void printLrTokenList(LrToken*lr);

