
#include"bss.h"
#include"token.h"

bool compBSS(CompContext*ctx){

  if (tokenIdentComp(".space",ctx->token)) {
    if(!ctx->token->next)
      compError("Unexpected EOF",ctx->token);
    ctx->token = ctx->token->next;
    uint32_t n = parseUInt(ctx->token);
    ctx->token = ctx->token->next;
    if(ctx->token && ctx->token->type != Newline)
      compError("Newline or EOF expected after .space <number> directive",ctx->token);
    if(ctx->pass == COMP){
      ctx->section->size += n;
      ctx->section->index += n;
    }
    return true;
  }

  return false;
}
