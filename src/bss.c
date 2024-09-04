
#include"common_types.h"
#include"bss.h"
#include"token.h"
#include"constants.h"

bool compBSS(CompContext*ctx){

  if (tokenIdentComp(".space",ctx->token)) {
    nextTokenEnforceExistence(ctx);

    Token*valTok = getNumberOrConstant(ctx);
    if(!valTok)
      compError("Number expected",ctx->token);
    uint32_t n = parseUInt(valTok);

    ctx->token = ctx->token->next;
    if(ctx->token && ctx->token->type != Newline)
      compError("Newline or EOF expected after .space <number> directive",ctx->token);

    if(ctx->pass == INDEX)
      ctx->section->size += n;
    if(ctx->pass == COMP){
      ctx->section->index += n;
    }

    return true;
  }

  return false;
}
