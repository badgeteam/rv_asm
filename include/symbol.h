#pragma once
#include"common_types.h"


void initSymbolList(CompContext*ctx);
void symbolPassPostIndex(CompContext*ctx);
void symbolPassPostComp(CompContext*ctx);

Symbol*getSymbol(CompContext*ctx,struct Token*nameToken);

bool tryCompSymbolDirectives(CompContext*ctx);



