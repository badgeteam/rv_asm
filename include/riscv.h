#pragma once

#include"common_types.h"

uint32_t parseIntReg(struct Token*token);
uint32_t parseFloatReg(struct Token*token);
uint32_t parseBracketReg(CompContext*ctx);

void insert4ByteCheckLineEnd(CompContext*ctx, uint32_t enc);

bool compRV(CompContext*ctx);

