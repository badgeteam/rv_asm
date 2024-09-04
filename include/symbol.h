#pragma once
#include"common_types.h"


void initSymbolList(CompContext*ctx);
void symbolPassPostIndex(CompContext*ctx);
void symbolPassPostComp(CompContext*ctx);

Symbol*getSymbol(CompContext*ctx,struct Token*nameToken);

void addSymbol(CompContext*ctx,struct Token*nameToken, uint32_t value, uint32_t size, uint32_t type, uint32_t bind, uint32_t vis, uint32_t shndx);


