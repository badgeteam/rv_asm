#pragma once
#include"common_types.h"

void addRelaEntry(CompContext*ctx,uint32_t offset, Symbol*sym, uint32_t type, int32_t addend);


bool tryCompImplicitRelocation(CompContext*ctx, uint32_t type);

bool tryCompRelocation(CompContext*ctx, uint32_t type);


