
#include"data.h"
#include"token.h"
#include"util.h"
#include"constants.h"
#include<stdio.h>

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

  Token*valTok = getNumberOrConstant(ctx);
  if(!valTok)
    compError("Number or existing Constant expected",ctx->token);
  uint32_t size = parseUInt(valTok);

  if(ctx->pass == INDEX)
    ctx->section->size += size;
  else{
    for(uint32_t i = 0;i<size;i++){
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
      Token*valTok = getNumberOrConstant(ctx);
      if(valTok)
	ctx->section->buff[ctx->section->index] = parseUImm(valTok,8);
      else if(ctx->token->type == Char)
	ctx->section->buff[ctx->section->index] = parseChar(ctx->token->buff + 1);
      else compError("Number, Constant or Char expected",ctx->token);
      ctx->section->index++;
    }
  }
  while(nextTokenCheckConcat(ctx));
}

void encodeBytes(CompContext*ctx,uint32_t log2size, bool flag_align){
  nextTokenEnforceExistence(ctx);

  if(flag_align){
    if(ctx->pass == INDEX)
      ctx->section->size = align(ctx->section->size, log2size);
    else{
      uint32_t align_mask = (1<<log2size)-1;
      while(ctx->section->index & align_mask){
	ctx->section->buff[ctx->section->index] = 0;
	ctx->section->index++;
      }
    }
  }

  Token*valTok;
  do{
    if(ctx->pass == INDEX){
      ctx->section->size += 1<<log2size;
    }
    else{
      if(!(valTok = getNumberOrConstant(ctx)))
	compError("Number or Constant expected",ctx->token);

      bool numberHasSign = *(valTok->buff) == '-';
      if(log2size==1){
	if(numberHasSign)
	  *((int16_t*)(ctx->section->buff + ctx->section->index)) = parseImm(valTok,16);
	else
	  *((uint16_t*)(ctx->section->buff + ctx->section->index)) = parseUImm(valTok,16);
      }
      else if(log2size){
	if(numberHasSign)
	  *((int32_t*)(ctx->section->buff + ctx->section->index)) = parseInt(valTok);
	else
	  *((uint32_t*)(ctx->section->buff + ctx->section->index)) = parseUInt(valTok);
      }
      ctx->section->index += 1<<log2size;
    }
  }
  while(nextTokenCheckConcat(ctx));
}

void encodeIncbin(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != String)
    compError("String expected",ctx->token);

  *(ctx->token->buffTop - 1) = '\0';
  FILE*fp = fopen(ctx->token->buff+1,"rb");
  *(ctx->token->buffTop - 1) = '"';

  if(!fp)compError("Unable to open File",ctx->token);

  fseek(fp,0L,SEEK_END);
  uint32_t filesize = ftell(fp);
  fseek(fp,0L,SEEK_SET);

  if(ctx->pass == INDEX){
    ctx->section->size += filesize;
  }else{
    fread(ctx->section->buff + ctx->section->index, 1, filesize, fp);
    ctx->section->index += filesize;
  }
  
  fclose(fp);
  ctx->token = ctx->token->next;
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

  else if(tokenIdentComp(".2byte",ctx->token))
    encodeBytes(ctx,1,false);

  else if(tokenIdentComp(".4byte",ctx->token))
    encodeBytes(ctx,2,false);

  else if(tokenIdentComp(".half",ctx->token))
    encodeBytes(ctx,1,true);

  else if(tokenIdentComp(".word",ctx->token))
    encodeBytes(ctx,2,true);

  else if(tokenIdentComp(".incbin",ctx->token))
    encodeIncbin(ctx);

  else return false;
  return true;

}


