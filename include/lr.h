#pragma once
#include"common_types.h"


#define DEBUG_LR

// Does advance after the last token
//bool lrParseExpression(CompContext*ctx, bool parse_symbol);
bool lrParseNumber(CompContext*ctx);
bool lrParseRelocation(CompContext*ctx);

uint32_t lrGetUInt(CompContext*ctx);
int32_t lrGetInt(CompContext*ctx);
uint32_t lrGetUImm(CompContext*ctx, uint32_t bits);
uint32_t lrGetImm(CompContext*ctx, uint32_t bits);
int32_t lrGetNumberSign(CompContext*ctx);

Token*lrGetSymbol(CompContext*ctx);
bool lrIsPcrel(CompContext*ctx);

void printLrToken(LrToken*lr);
void printLrTokenList(LrToken*lr);

