
#include"comp.h"
#include"util.h"

void compPass(CompContext*ctx){
  while(ctx->token){
    switch(ctx->token->type){
      case Space:
      case Comment:
      case Newline:
	ctx->token=ctx->token->next;
	break;
      case Identifier:
	if(ctx->token->next && ctx->token->next->type == Doubledot){
	  // Symbol
	}else if(ctx->token->buff[0] == '.'){
	  // Directive
	}else{
	  // Statements
	}
      default:
	compError("Unexpected Token in Main Switch",ctx->token);
    }
  }
}


CompContext*comp(char*filename){
  CompContext*ctx = malloc(sizeof(CompContext));
  ctx->tokenHead = tokenizeFile(filename);
  printTokenInfo(ctx->tokenHead);
  ctx->tokenHead = pruneTokenTypes(ctx->tokenHead,Space|Comment);

  printTokenInfo(ctx->tokenHead);
//  ctx->token = ctx->tokenHead;
//  ctx->pass = INDEX;

//  compPass(ctx);

//  ctx->token = ctx->tokenHead;
//  ctx->pass = COMP;

//  compPass(ctx);

  return ctx;
}


