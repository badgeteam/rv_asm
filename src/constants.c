
#include"constants.h"
#include"token.h"
#include<stdlib.h>

void addConstant(CompContext*ctx,Token*nameToken,Token*valueToken){
  if(nameToken->type != Identifier)
    compError("Identifier expected",nameToken);
  if(valueToken->type != Number)
    compError("Number expected",valueToken);
  Constant*con = malloc(sizeof(Constant));
  con->nameToken = nameToken;
  con->valueToken = valueToken;
  con->next = ctx->constantHead;
  ctx->constantHead = con;
}

Token*getConstant(CompContext*ctx, Token*nameToken){
  for(Constant*con = ctx->constantHead; con; con=con->next)
    if(tokenComp(con->nameToken,nameToken))
      return con->valueToken;
  return NULL;
}

Token*getNumberOrConstant(CompContext*ctx){
  if(ctx->token->type == Number)
    return ctx->token;
  else if(ctx->token->type == Identifier)
    return getConstant(ctx,ctx->token);
  else
   return NULL;
}
