
#include"lr.h"
#include"token.h"
#include"constants.h"
#include<stdlib.h>

#include<stdio.h>

void lrPush(CompContext*ctx, LrType type, uint32_t value, int sign, Token*token){
  LrToken*lr;
  if((lr=ctx->lrUnused)){
    ctx->lrUnused = lr->next;
    lr->next = ctx->lrHead;
    ctx->lrHead = lr;
  }
  else{
    lr = malloc(sizeof(LrToken));
    lr->next = ctx->lrHead;
    ctx->lrHead = lr;
  }
  lr->type = type;
  lr->value = value;
  lr->sign = sign;
  lr->token = token;
}

void lrPop(CompContext*ctx){
  LrToken*lr = ctx->lrHead;
  if(!lr)return;
  ctx->lrHead = lr->next;
  lr->next = ctx->lrUnused;
  ctx->lrUnused = lr;
}

bool lrReduceBrackets(CompContext*ctx){
  LrToken*lr = ctx->lrHead;
  if( !lr || lr->type != Lr_BracketOut )
    return false;
  lr = lr->next;
  if( !lr || lr->type != Lr_Number )
    return false;
  uint32_t value = lr->value;
  lr = lr->next;
  if( !lr || lr->type != Lr_BracketIn )
    return false;
  lrPop(ctx);
  lrPop(ctx);
  lr->value = value;
  lr->type = Lr_Number;
  lr->token = NULL;
  return true;
}

bool lrReduceBinaryOperator(CompContext*ctx, LrType type){
  LrToken*lr = ctx->lrHead;
  if( !lr || lr->type != Lr_Number )
    return false;
  uint32_t v2 = lr->value;
  int s2 = lr->sign;
  lr = lr->next;
  if( !lr || lr->type != type )
    return false;
  lr = lr->next;
  if( !lr || lr->type != Lr_Number )
    return false;
  uint32_t v1 = lr->value;
  int s1 = lr->sign;

  switch(type){

    case Lr_Add:
      if(s1 == s2){
	lr->value = v1 + v2;
	lr->sign = s1;
      }else if(s2 == 1){
	lr->value = v1 - v2;
	lr->sign = (v2 >= v1) ? 1 : -1;
      }else{
	lr->value = v2 - v1;
	lr->sign = (v1 >= v2) ? 1 : -1;
      }
      break;

    case Lr_Sub:
      if( s1 == 1 && s2 == 1){
	lr->value = v1 - v2;
	lr->sign = (v1 >= v2) ? 1 : -1;
      }else if(s1 == 1 && s2 == -1){
	lr->value = v1 + v2;
	lr->sign = 1;
      }else if(s1 == -1 && s2 == 1){
	lr->value = - v1 - v2;
	lr->sign = -1;
      }else{
	lr->value = v2 - v1;
	lr->sign = (v2 >= v1) ? 1 : -1;
      }

      break;

    case Lr_Mul:
      lr->value = v1 * v2;
      lr->sign = s1 * s2;
      break;

    case Lr_Div:
      if(v2 == 0) compError("Dividing by zero",ctx->token);
      lr->value = v1 / v2;
      lr->sign = s1 * s2;
      break;

    case Lr_Rem:
      lr->value = v1 % v2;
      lr->sign = s1;
      break;

    default:
      return false;
  }
  lr->value = lr->sign == 1 ? lr->value : - lr->value;
  lrPop(ctx);
  lrPop(ctx);
  return true;
}

bool lrReduceUnaryOperator(CompContext*ctx, LrType type){
  LrToken*lr = ctx->lrHead;
  if( !lr || lr->type != Lr_Number )
    return false;
  uint32_t value = lr->value;
  int sign = lr->sign;
  Token*token = lr->token;
  lr = lr->next;
  if(!lr || lr->type != type)
    return false;
  if(type != Lr_Sub)
    return false;
  lr->value = value;
  lr->sign = - sign;
  lr->token = token;
  lr->type = Lr_Number;
  lrPop(ctx);
  return true;
}

bool lrReduceConstant(CompContext*ctx){
  if(ctx->lrHead && ctx->lrHead->type == Lr_Constant){
    Constant*con = getConstant(ctx,ctx->lrHead->token);
    ctx->lrHead->value = con->value;
    ctx->lrHead->sign = con->sign;
    ctx->lrHead->token = NULL;
    ctx->lrHead->type = Lr_Number;
    return true;
  }
  return false;
}

LrType lrLookahead(CompContext*ctx){
  if(!ctx->token) return 0;
  switch(ctx->token->type){
    case Number:
      return Lr_Number;
    case Identifier:
      if(getConstant(ctx,ctx->token))
	return Lr_Constant;
      return Lr_Symbol;
    case BracketIn:
      return Lr_BracketIn;
    case BracketOut:
      return ctx->lrBracketDepth == 0 ? 0 : Lr_BracketOut;
    case Plus:
      return Lr_Add;
    case Minus:
      return Lr_Sub;
    case Times:
      return Lr_Mul;
    case Slash:
      return Lr_Div;
    case Percent:
      return Lr_Rem;
    default:
      return 0;
  }
}

bool lrShift(CompContext*ctx){
  LrType type = lrLookahead(ctx);
  if( type == 0 ) return false;

  if( type == Lr_Number )
    lrPush(ctx, Lr_Number, parseNumber(ctx->token), 1, NULL);

  else if( type == Lr_Constant ){
    Constant*con = getConstant(ctx,ctx->token);
    lrPush(ctx,Lr_Constant, 0, 0, ctx->token);
  }

  else if( type == Lr_Symbol )
    lrPush(ctx, Lr_Symbol, 1, 1, ctx->token);

  else if( type == Lr_BracketIn ){
    ctx->lrBracketDepth++;
    lrPush(ctx, Lr_BracketIn, 0, 0, NULL);
  }

  else if( type == Lr_BracketOut ){
    // Zero heck happens in lrLookahead
    ctx->lrBracketDepth--;
    lrPush(ctx, Lr_BracketOut, 0, 0, NULL);
  }

  else
    lrPush(ctx, type, 0, 0, NULL);

  ctx->token = ctx->token->next;
  return true;
}


bool lrNumConstAccept(CompContext*ctx){
  // TODO Accept Symbols
  if(lrLookahead(ctx) != 0) return false;
  if(!ctx->lrHead) return false;
  if(ctx->lrHead->next) return false;
  if(ctx->lrHead->type != Lr_Number) return false;
  return true;
}


bool lrParseNumConstExpression(CompContext*ctx){
  // Cleanup Old Stuff
  ctx->lrBracketDepth = 0;
  while(ctx->lrHead)
    lrPop(ctx);
  

  Token*backupToken = ctx->token;

  // LR1
  while(true){

    printLrTokenList(ctx->lrHead);

    if(lrReduceConstant(ctx)) continue;
    if(lrReduceBrackets(ctx)) continue;


    LrType type_lookahead = lrLookahead(ctx);

    if(type_lookahead == Lr_Rem){
      lrShift(ctx);
      continue;
    }
    if(lrReduceBinaryOperator(ctx,Lr_Rem)) continue;

    if(type_lookahead == Lr_Div){
      lrShift(ctx);
      continue;
    }
    if(lrReduceBinaryOperator(ctx,Lr_Div)) continue;

    if(type_lookahead == Lr_Mul){
      lrShift(ctx);
      continue;
    }
    if(lrReduceBinaryOperator(ctx,Lr_Mul)) continue;

    if(type_lookahead == Lr_Sub){
      lrShift(ctx);
      continue;
    }
    if(lrReduceBinaryOperator(ctx,Lr_Sub)) continue;

     if(type_lookahead == Lr_Add){
      lrShift(ctx);
      continue;
    }
    if(lrReduceBinaryOperator(ctx,Lr_Add)) continue;

    if(lrReduceUnaryOperator(ctx,Lr_Sub)) continue;

    if(lrShift(ctx))
      continue;

    break;
  }

//  printLrTokenList(ctx->lrHead);

  if(lrNumConstAccept(ctx)){
    printf("Lr Accepted Value = %d Sign = %d\n\n", ctx->lrHead->value, ctx->lrHead->sign);

    return true;
  }
  ctx->token = backupToken;
  return false;

}

uint32_t getUInt(CompContext*ctx){
  if(!ctx->lrHead || ctx->lrHead->sign != 1)
    compError("Result of expression is negative",ctx->token);
  return ctx->lrHead->value;
}

int32_t getInt(CompContext*ctx){
  if(!ctx->lrHead || ctx->lrHead->value & (1<<31))
    compError("Value out of range",ctx->token);
  return ctx->lrHead->value * ctx->lrHead->sign;
}

uint32_t getUImm(CompContext*ctx, uint32_t bits){
  uint32_t n = getUInt(ctx);
  if(n >= (1<<bits))
    compError("Value out of range",ctx->token);
  return n;
}

uint32_t getImm(CompContext*ctx, uint32_t bits){
  int32_t n = getInt(ctx);
  //TODO Bounds Check

  return n & ( ( 1 << bits ) -1 );
}


char*getLrTypeName(LrType type){
  switch(type){
    case Lr_Constant: 	return "Constant    ";
    case Lr_Number: 	return "Number      ";
    case Lr_Symbol: 	return "Symbol      ";
//    case Lr_DotSymbol: 	return "DotSymbol   ";
    case Lr_BracketIn: 	return "Bracket In  ";
    case Lr_BracketOut: return "Bracket Out ";
    case Lr_Add: 	return "Add         ";
    case Lr_Sub: 	return "Sub         ";
    case Lr_Mul: 	return "Mul         ";
    case Lr_Div: 	return "Div         ";
    case Lr_Rem: 	return "Rem         ";
    default: 		return "Unknown LR Type";
  }
}

void printLrToken(LrToken*lr){
  printf("Lr Token: Type = %s ",getLrTypeName(lr->type));
  if(lr->type & ( Lr_Constant | Lr_Symbol)){
    if(lr->token)
      for(char*cp = lr->token->buff; cp < lr->token->buffTop; cp++)
	printf("%c",*cp);

  }
  else if(lr->type == Lr_Number){
    printf("Value = %d \tSign = %d",lr->value,lr->sign);
  }
  printf("\n");
}

void printLrTokenList(LrToken*lr){
  printf("Lr Token List\n");
  while(lr){
    printLrToken(lr);
    lr = lr->next;
  }
  printf("\n");
}
