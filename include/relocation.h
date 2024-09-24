#pragma once
#include"common_types.h"

void addRelaEntry(CompContext*ctx,uint32_t offset, Symbol*sym, uint32_t type, int32_t addend);

/* Does not advance after the last token
 *
 */
bool tryCompRelocation(CompContext*ctx, uint32_t type);


