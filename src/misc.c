#include"misc.h"
#include"token.h"
#include"lr.h"
#include"util.h"

bool tryCompMiscDirectives(CompContext*ctx){
  
  if(tokenIdentComp(".align",ctx->token)){
    if(!ctx->section)
      compError("No Section selected",ctx->token);
    nextTokenEnforceExistence(ctx);
    if(!lrParseNumber(ctx))
      compError("Arithmetic Expression expeted",ctx->token);
    enforceNewlineEOF(ctx);
    if(ctx->pass==INDEX){
      ctx->section->size = align(ctx->section->size, lrGetUImm(ctx, 5));
    }
    else if(ctx->section->shdr.sh_type == SHT_NOBITS){
      ctx->section->index = align(ctx->section->index, lrGetUImm(ctx, 5));
    }
    else{
      uint32_t align_mask = (1 << lrGetUImm(ctx, 5)) -1;
      while(ctx->section->index & align_mask){
        ctx->section->buff[ctx->section->index] = 0;
        ctx->section->index ++;
      }
    }
    return true;
  }


  return false;
}
