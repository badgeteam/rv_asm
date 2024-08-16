#include"data.h"

void compString(CompContext*ctx){
  char c;
  for(char*cp = ctx->token->buff + 1; cp<ctx->token->buffTop - 1;cp++){
    if(*cp == '\\'){
      cp++;
      if(ctx->pass == INDEX){
	ctx->section->size ++;
      }else if(ctx->pass == COMP){
	c = *cp;
	switch(c){
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
	ctx->section->buff[ctx->section->index] = c;
	ctx->section->index ++;
      }
    }else{
      if(ctx->pass == INDEX){
	ctx->section->size ++;
      }else if(ctx->pass == COMP){
	ctx->section->buff[ctx->section->index] = *cp;
	ctx->section->index ++;
      }
    }
  }
  ctx->token = ctx->token->next;
}


bool compData(CompContext*ctx){
  if(tokenIdentComp(".ascii",ctx->token)){
    ctx->token = ctx->token->next;
    while(ctx->token && ctx->token->type & String){
      compString(ctx);
    }
    return true;
  }

  if(tokenIdentComp(".string",ctx->token)){
    ctx->token = ctx->token->next;
    while(ctx->token && ctx->token->type & String){
      compString(ctx);
    }
      if(ctx->pass == INDEX)
	ctx->section->size ++;
      else if(ctx->pass == COMP){
	ctx->section->buff[ctx->section->index] = '\0';
	ctx->section->index ++;
      }
    return true;
  }

  if(tokenIdentComp(".zero",ctx->token)){
    if(!ctx->token->next)
      compError("Unexpected EOF",ctx->token);
    ctx->token = ctx->token->next;
    if(ctx->token->type != Number)
      compError("Number Expected after .zero",ctx->token);
    if(ctx->pass == INDEX)
      ctx->section->size += parseUInt(ctx->token);
    else if(ctx->pass == COMP){
      uint32_t number_of_zeros = parseUInt(ctx->token);
      for(uint32_t i = 0;i<number_of_zeros;i++){
	ctx->section->buff[ctx->section->index] = 0;
	ctx->section->index++;
      }
    }
    ctx->token = ctx->token->next;
    return true;
  }

//  if(tokenIdentComp(".byte",ctx->token)){
//    if(!ctx->token->next)
//      compError("Unexpecdet EOF",ctx->token);
//    ctx->token = ctx->token->next;
//    while(ctx->token && ctx->token->type & Number | Char){
//      if(ctx->pass == INDEX)
//	ctx->section->size ++;
//      else if(ctx->pass == COMP){
//	if(ctx->token->type == Number){
//	  ctx->section->buff[ctx->section->index] = (uint8_t)parseUImm(ctx->token,8);
//	}else if(ctx->token->type == Char){
//	  ctx->section->buff[ctx->section->index] = (uint8_t)parseChar(ctx->token->buff+1);
//	}
//	ctx->section->index++;
 //     }
//      ctx->token = ctx->token->next;
//      if(!ctx->token)
//	return true;
//      if(! (ctx->token->type & Colon||Newline) )
//	compError("Colon or Newline expected after byte in .byte",ctx->token);
//      ctx->token = ctx->token->next;
//    }
//  }

  return false;
}


