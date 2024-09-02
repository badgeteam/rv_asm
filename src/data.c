
#include"data.h"
#include"token.h"
#include"util.h"

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

void encodeBytes(CompContext*ctx,uint32_t size,bool flag_align){
  nextTokenEnforceExistence(ctx);

  if(flag_align){
    if(ctx->pass == INDEX)
      ctx->section->size = align(ctx->section->size, size);
    else{
      uint32_t align_mask = (1<<size)-1;
      while(ctx->section->index & align_mask){
	ctx->section->buff[ctx->section->index] = 0;
	ctx->section->index++;
      }
    }
  }

  do{
    if(ctx->token->type != Number)
      compError("Number expected",ctx->token);
    if(ctx->pass == INDEX){
      ctx->section->size += size;
    }
    else{
      //TODO
      if(size==2){}
      else if(size==4){}
      ctx->section->index += size;
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
    encodeBytes(ctx,2,false);

  else if(tokenIdentComp(".4byte",ctx->token))
    encodeBytes(ctx,4,false);

  else if(tokenIdentComp(".half",ctx->token))
    encodeBytes(ctx,2,true);

  else if(tokenIdentComp(".word",ctx->token))
    encodeBytes(ctx,4,true);

  else if(tokenIdentComp(".incbin",ctx->token))
    encodeIncbin(ctx);

  else return false;
  return true;

}


