
#include"constants.h"
#include"token.h"
#include"lr.h"
#include<stdlib.h>


void setConstant(CompContext*ctx,Token*nameToken, uint32_t value, int sign){
  Constant*con;
  if(!(con=getConstant(ctx,nameToken))){
    con = malloc(sizeof(Constant));
    con->next = ctx->constantHead;
    ctx->constantHead = con;
  }
  con->nameToken = nameToken;
  con->value = value;
  con->sign = sign;
}



bool tryCompEquSet(CompContext*ctx){
  if( ! (tokenIdentComp(".equ", ctx->token) || tokenIdentComp(".set", ctx->token) ) )
      return false;
  nextTokenEnforceExistence(ctx);
  Token*nameToken = ctx->token;
  if(nameToken->type != Identifier)
    compError("Identifier expected",nameToken);
  nextTokenEnforceComma(ctx);
  if(!lrParseNumConstExpression(ctx))
    compError("Unable to parse Expression",ctx->token);
  enforceNewlineEOF(ctx);
  setConstant(ctx,nameToken,ctx->lrHead->value,ctx->lrHead->sign);
  return true;
}

Constant*getConstant(CompContext*ctx, Token*nameToken){
  for(Constant*con = ctx->constantHead; con; con=con->next)
    if(tokenComp(con->nameToken,nameToken))
      return con;
  return NULL;
}
