#pragma once
#include"comp.h"
#include"util.h"

uint32_t parseIntReg(struct Token*token);
uint32_t parseFloatReg(struct Token*token);
uint32_t parseBracketReg(CompContext*ctx);

bool tryCompRelocation(CompContext*ctx,uint32_t type);

void nextTokenEnforceExistence(CompContext*ctx);
void nextTokenEnforceComma(CompContext*ctx);
bool compRV(CompContext*ctx);

