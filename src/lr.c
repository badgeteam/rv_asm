
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
  LrToken*lr_cursor = ctx->lrHead;
  if(!lr_cursor || lr_cursor->type != Lr_BracketOut) return false;
  lr_cursor = lr_cursor->next;
  LrToken*lr_number = lr_cursor;
  if(!lr_cursor || lr_cursor->type != Lr_Number) return false;
  lr_cursor = lr_cursor->next;
  if(!lr_cursor || lr_cursor->type != Lr_BracketIn) return false;
  lr_number->next = lr_cursor->next;
  lr_cursor->next = ctx->lrUnused;
  ctx->lrUnused = lr_cursor;
  lrPop(ctx);
  return true;
}

#if 0
bool lrReduceBrackets(CompContext*ctx){
  bool found_num=false;
  bool found_sym=false;
  bool found_dot=false;
  bool bracket_content = false;
  LrToken*lr = ctx->lrHead;
  if(!lr || lr->type != Lr_BracketOut) return false;
  
  // Advance through value tokens of unique type
  // Halt on last value token
  while(lr->next && lr->next->type & (Lr_Mask_Value)){
    if     ( !found_num && lr->next->type == Lr_Number )
      found_num = true;
    else if( !found_sym && lr->next->type == Lr_Symbol )
      found_sym = true;
    else if( !found_dot && lr->next->type == Lr_DotSymbol )
      found_dot = true;
    else return false;
    bracket_content = true;
    lr = lr->next;
  }
  // Enforce Bracket Content
  if(!bracket_content) return false;
  // Check for Bracket In
  if(!lr->next || lr->next->type != Lr_BracketIn) return false;
  LrToken*lr_bracket_in = lr->next;
  // Remove Bracket in
  lr->next = lr_bracket_in->next;
  lr_bracket_in->next = ctx->lrUnused;
  ctx->lrUnused = lr_bracket_in;
  // Remove Bracket out
  lrPop(ctx);
  return true;
}
#endif

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
  //lr->value = lr->sign == 1 ? lr->value : - lr->value;
  lrPop(ctx);
  lrPop(ctx);
  return true;
}

// Reduce Preceeding Sign into Value Token
bool lrReduceUnaryOperator(CompContext*ctx){
  LrToken*lr_value = ctx->lrHead;
  if(!lr_value) return false;
  if(!(lr_value->type & Lr_Mask_Value)) return false;

  LrToken*lr_sign = lr_value->next;
  if(!lr_sign) return false;
  if(! (lr_sign->type & (Lr_Add | Lr_Sub)) ) return false;

  if(lr_sign->type == Lr_Sub)
    lr_value->sign *= -1;

  lr_value->next = lr_sign->next;
  lr_sign->next = ctx->lrUnused;
  ctx->lrUnused = lr_sign;

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

bool lrReduceMoveSym(CompContext*ctx, LrType sym_type){
  LrToken*lr_advance = ctx->lrHead;
  LrToken*lr_pre_move = NULL;
  LrToken*lr_move = NULL;

  // Iterate over Addition Chain, stop at sym_type
  while( lr_advance
      && lr_advance->type & (Lr_Mask_Value | Lr_Add | Lr_Sub)
      && lr_advance->type != sym_type
      )
  {
    lr_pre_move = lr_advance;
    lr_advance = lr_advance->next;
  }

  // Check for sym_type
  if( !lr_advance || lr_advance->type != sym_type) return false;
  lr_move = lr_advance;
  
  // Advance toward end of Addition Chain
  // Error Upon multiple sym_type
  while( lr_advance->next
      && lr_advance->next->type & (Lr_Mask_Value | Lr_Add | Lr_Sub)
      )
  {
    lr_advance = lr_advance->next;
    if(lr_advance->type == sym_type)
      compError("Multiple Symbols inside an Expression",ctx->token);
  }

  // Check if Symbol has to be moved
  if(lr_advance == lr_move)
    return false;

  // Move Symbol
  if(lr_pre_move)
    lr_pre_move->next = lr_move->next;
  lr_move->next = lr_advance->next;
  lr_advance->next = lr_move;

  return true;
}


LrType lrLookahead(CompContext*ctx){
  if(!ctx->token) return 0;
  switch(ctx->token->type){
    case Number:
      return Lr_Number;
    case Identifier:
      if(tokenStrComp(".",ctx->token))
	return Lr_DotSymbol;
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
    lrPush(ctx,Lr_Constant, 0, 0, ctx->token);
  }

  else if( type == Lr_Symbol )
    lrPush(ctx, Lr_Symbol, 0, 1, ctx->token);

  else if( type == Lr_DotSymbol )
    lrPush(ctx, Lr_DotSymbol, 0, 1, ctx->token);

  else if( type == Lr_BracketIn ){
    ctx->lrBracketDepth++;
    lrPush(ctx, Lr_BracketIn, 0, 0, NULL);
  }

  else if( type == Lr_BracketOut ){
    // Zero check happens in lrLookahead
    ctx->lrBracketDepth--;
    lrPush(ctx, Lr_BracketOut, 0, 0, NULL);
  }

  else
    lrPush(ctx, type, 0, 0, NULL);

  ctx->token = ctx->token->next;
  return true;
}

// { { [Dot] } [Symbol] } [Number]
bool lrAccept(CompContext*ctx, bool parse_symbol){
  LrToken*tok = ctx->lrHead;
  if(!tok) return false;
  if(tok->type != Lr_Number) return false;

  tok = tok->next;
  if(!parse_symbol && !tok) return true;

  if(!tok) return false;
  if(tok->type != Lr_Symbol && tok->sign != 1) return false;

  tok = tok->next;
  if(!tok) return true;
  if(tok->type != Lr_DotSymbol && tok->sign != -1) return false;

  if(tok->next) return false;
  return true;
}


bool lrParseExpression(CompContext*ctx, bool parse_symbol){
  // Cleanup Old Stuff
  ctx->lrBracketDepth = 0;
  while(ctx->lrHead)
    lrPop(ctx);
  
  Token*backupToken = ctx->token;

  // LR1
  while(true){

#ifdef DEBUG_LR
    printLrTokenList(ctx->lrHead);
#endif

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

    if(lrReduceUnaryOperator(ctx)) continue;
    if(lrReduceUnaryOperator(ctx)) continue;

    if(lrReduceMoveSym(ctx, Lr_Symbol)) continue;
    if(lrReduceMoveSym(ctx, Lr_DotSymbol)) continue;

    if(lrShift(ctx))
      continue;

    break;
  }

#ifdef DEBUG_LR
  printLrTokenList(ctx->lrHead);
#endif

  if(lrAccept(ctx, parse_symbol)){

#ifdef DEBUG_LR
  printf("Lr Accepted\n\n");
#endif

    return true;
  }
  ctx->token = backupToken;
  return false;

}

uint32_t lrGetUInt(CompContext*ctx){
  if(!ctx->lrHead)
    compError("No Number was parsed",ctx->token);
  if(ctx->lrHead->sign != 1)
    compError("Result of expression is negative",ctx->token);
  return ctx->lrHead->value;
}

int32_t lrGetInt(CompContext*ctx){
  if(!ctx->lrHead)
    compError("No Number was Parsed",ctx->token);
  if(ctx->lrHead->value & (1<<31))
    compError("Value out of range",ctx->token);
  return ctx->lrHead->value * ctx->lrHead->sign;
}

uint32_t lrGetUImm(CompContext*ctx, uint32_t bits){
  uint32_t n = lrGetUInt(ctx);
  if(n >= (1<<bits))
    compError("Value out of range",ctx->token);
  return n;
}

uint32_t lrGetImm(CompContext*ctx, uint32_t bits){
  int32_t n = lrGetInt(ctx);
  //TODO Bounds Check
  if(false)
    compError("Value out of range",ctx->token);
  return n & ( ( 1 << bits ) -1 );
}

Token*lrGetSymbol(CompContext*ctx){
  LrToken*lr = ctx->lrHead;
  if(!lr || !lr->next)
    compError("No Symbol was parsed",ctx->token);
  return lr->next->token;
}

bool lrIsPcrel(CompContext*ctx){
  LrToken*lr = ctx->lrHead;
  if(!lr || !lr->next)
    compError("No Symbol was parsed",ctx->token);
  return lr->next->next != NULL;
}


#ifdef DEBUG_LR

char*getLrTypeName(LrType type){
  switch(type){
    case Lr_Constant: 	return "Constant    ";
    case Lr_Number: 	return "Number      ";
    case Lr_Symbol: 	return "Symbol      ";
    case Lr_DotSymbol: 	return "DotSymbol   ";
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
  // Print Type
  printf("Lr Token: Type = %s",getLrTypeName(lr->type));

  // Print Token
  if(lr->type & ( Lr_Constant | Lr_Symbol) ){
    printf("\tName = ");
    if(lr->token)
      for(char*cp = lr->token->buff; cp < lr->token->buffTop; cp++)
	printf("%c",*cp);
  }

  // Print Number
  else if(lr->type == Lr_Number){
    printf("\tValue = %d",lr->value);
  }
  
  // Print Sign
  if(lr->type & ( Lr_Number | Lr_Symbol | Lr_DotSymbol ) ){
    printf("\tSign = %d",lr->sign);
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

#endif
