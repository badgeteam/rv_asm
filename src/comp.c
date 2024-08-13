
#include"comp.h"
#include"util.h"

void compPass(CompContext*ctx){
  bool mode_text=false,mode_data=false;
  while(ctx->token){
    if(ctx->token->type == Newline){
      ctx->token = ctx->token->next;
    }
    else if(ctx->token->type == Identifier){
      if(ctx->token->next && ctx->token->next->type == Doubledot){
      // Symbol
      // TODO Add Symbol
      ctx->token = ctx->token->next->next;
      break;
      }
      // Directive
      else if(tokenIdentComp(".text",ctx->token)){
	// TODO Select Section
  printf(".text\n");
	mode_text = true;
	ctx->token=ctx->token->next;
      }

      else if(mode_text){ctx->token = ctx->token->next;}
      else if(mode_data){ctx->token = ctx->token->next;}
      else
	compError("Unexpected Token in Main Switch",ctx->token);
    }
    else {
      compError("Unexpected Token in Main Switch",ctx->token);
    }
  }
}


CompContext*comp(char*filename){
  CompContext*ctx = malloc(sizeof(CompContext));
  ctx->tokenHead = tokenizeFile(filename);
  ctx->tokenHead = pruneTokenTypes(ctx->tokenHead,Space|Comment);
  printTokenInfo(ctx->tokenHead);

  ctx->token = ctx->tokenHead;
  ctx->pass = INDEX;

  compPass(ctx);

//  ctx->token = ctx->tokenHead;
//  ctx->pass = COMP;

//  compPass(ctx);

  return ctx;
}


