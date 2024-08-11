
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

  for(struct Token*token = ctx->tokenHead;token;token=token->next){
    printf("Token Type=%s\tAddress=0x%lx\tSize=0x%lx\tContent=",
	tokenTypeName(token),(size_t)token->buff,(size_t)(token->buffTop-token->buff));
    for(char*cp = token->buff;cp<token->buffTop;cp++){
      printf("%c",*cp);
    }
    printf("\n");
  }

//  ctx->token = ctx->tokenHead;
//  ctx->pass = INDEX;

//  compPass(ctx);

//  ctx->token = ctx->tokenHead;
//  ctx->pass = COMP;

//  compPass(ctx);

  return ctx;
}


