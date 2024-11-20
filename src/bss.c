
#include"common_types.h"
#include"bss.h"
#include"token.h"
#include"lr.h"

bool compBSS(CompContext*ctx){

  if (tokenIdentComp(".space",ctx->token)) {
    nextTokenEnforceExistence(ctx);
  
    if(!lrParseNumber(ctx))
      compError("Arithmetic Expression expected",ctx->token);
    enforceNewlineEOF(ctx);

    if(ctx->pass == INDEX)
      ctx->section->size += lrGetUInt(ctx);
    if(ctx->pass == COMP){
      ctx->section->index += lrGetInt(ctx);
    }

    return true;
  }

  return false;
}

