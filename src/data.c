#include"data.h"

bool nextTokenCheckConcat(CompContext*ctx){
  ctx->token = ctx->token->next;
  if(!ctx->token || ctx->token->type == Newline)
    return false;

  if(ctx->token->type == Comma){
    nextTokenEnforceExistence(ctx);
    return true;
  }

  compError("Comma, Newline or EOF expected",ctx->token);
  return false;
}


void encodeAscii(CompContext*ctx, bool zero_terminated){
  nextTokenEnforceExistence(ctx);
  char c;
  do {
    if(ctx->token->type != String)
      compError("String expected",ctx->token);

    for(char*cp = ctx->token->buff + 1; cp < ctx->token->buffTop -1; cp++){
      
      if((c=*cp)=='\\'){
	cp++;
	switch(*cp){
	  case'0':c = '\0';break;
	  case'a':c = '\a';break;
	  case'b':c = '\b';break;
	  case't':c = '\t';break;
	  case'n':c = '\n';break;
	  case'v':c = '\v';break;
	  case'f':c = '\f';break;
	  case'r':c = '\r';break;
	  case'\\':c = '\\';break;
	  case'"':c = '"';break;
	}
      }

      if(ctx->pass == INDEX)
	ctx->section->size++;
      else{
	ctx->section->buff[ctx->section->index] = c;
	ctx->section->index++;
      }

    }
    
    if(zero_terminated){
      if(ctx->pass == INDEX)
	ctx->section->size++;
      else{
	ctx->section->buff[ctx->section->index] = '\0';
	ctx->section->index++;
      }
    }

  }
  while(nextTokenCheckConcat(ctx));
}

void encodeZeros(CompContext*ctx){
  nextTokenEnforceExistence(ctx);

  if(ctx->token->type != Number)
    compError("Number expected after .zero directive",ctx->token);

  if(ctx->pass == INDEX)
    ctx->section->size += parseUInt(ctx->token);
  else{
    uint32_t zeros = parseUInt(ctx->token);
    for(uint32_t i = 0;i<zeros;i++){
      ctx->section->buff[ctx->section->index] = 0;
      ctx->section->index ++;
    }
  }

  ctx->token = ctx->token->next;
}

void encodeByte(CompContext*ctx){
  nextTokenEnforceExistence(ctx);

  do{
    if(ctx->pass == INDEX)
      ctx->section->size ++;
    else{
      if(ctx->token->type == Number){
	ctx->section->buff[ctx->section->index] = parseUImm(ctx->token,8);
      }else if(ctx->token->type == Char){
	ctx->section->buff[ctx->section->index] = parseChar(ctx->token->buff + 1);
      }
      else compError("Number or Char expected",ctx->token);
      ctx->section->index++;
    }
  }
  while(nextTokenCheckConcat(ctx));
}

bool compData(CompContext*ctx){
  if(tokenIdentComp(".ascii",ctx->token))
    encodeAscii(ctx,false);

  else if(tokenIdentComp(".string",ctx->token))
    encodeAscii(ctx,true);

  else if(tokenIdentComp(".zero",ctx->token))
    encodeZeros(ctx);

  else if(tokenIdentComp(".byte",ctx->token))
    encodeByte(ctx);

  else return false;
  return true;

}


