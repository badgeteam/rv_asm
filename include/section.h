#pragma once

#include"common_types.h"

Section*addSection(CompContext*ctx,char*name,uint32_t type,uint32_t flags,
    uint32_t link,uint32_t info,uint32_t entsize,uint32_t addralign);

Section*getSectionByIdentifier(CompContext*ctx);

bool tryCompSectionDirectives(CompContext*ctx);
