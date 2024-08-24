
#include"riscv.h"

// only used by parseIntReg and parseFloatReg
uint32_t parseRegNum(struct Token*token,uint32_t offset){
  if(token->buffTop - token->buff <= offset){
    compError("Unable to parse Reg Num",token);
  }
  uint32_t reg = 0;
  char c;
  for(char*cp = token->buff + offset; cp<token->buffTop; cp++){
    c = *cp;
    if(c < '0' || c > '9')
      compError("Unexpected char in Reg Num",token);
    reg = reg * 10 + c - '0';
  }
  return reg;
}

uint32_t parseIntReg(struct Token*token){
  if(tokenIdentComp("zero",token))return 0;
  if(tokenIdentComp("ra",token))return 1;
  if(tokenIdentComp("sp",token))return 2;
  if(tokenIdentComp("gp",token))return 3;
  if(tokenIdentComp("tp",token))return 4;

  char c = *(token->buff);
  uint32_t reg = parseRegNum(token,1);

  if(c == 'x' || c == 'X'){
    if(reg<32)return reg;
    compError("x register must be smaller than 32",token);
  }
  if(c == 't' || c == 'T'){
    if(reg<=2)return reg + 5;
    if(reg<=6)return reg + 25;
    compError("t register must be smaller than 7",token);
  }
  if(c == 'a' || c == 'A'){
    if(reg<=7)return reg + 10;
    compError("a register must be smaller than 8",token);
  }
  if(c == 's' || c == 'S'){
    if(reg<=1)return reg + 8;
    if(reg<=11)return reg + 16;
    compError("s register must be smaller than 12",token);
  }
  compError("Unknown RiscV Register",token);
  return 0;
  
}

uint32_t parseFloatReg(struct Token*token){
  if( (*(token->buff) != 'f') || token->buff+1 >= token->buffTop)
    compError("Unknown RiscV Float Register",token);
  char c = *(token->buff+1);
  uint32_t reg;
  if(c == 't' || c == 'T'){
    reg = parseRegNum(token,2);
    if(reg <= 7)return reg;
    if(reg <= 11)return reg + 20;
    compError("ft register must be smaller than 12",token);
  }
  if(c == 'a' || c == 'A'){
    reg = parseRegNum(token,2);
    if(reg <= 7)return reg + 10;
    compError("fa register must be smaller than 8",token);
  }
  if(c == 's' || c == 'S'){
    reg = parseRegNum(token,2);
    if(reg <= 1)return reg + 8;
    if(reg <= 11)return reg + 16;
    compError("fs registster must be smaller than 12",token);
  }
  reg = parseRegNum(token,1);
  if(reg<32)return reg;
  compError("f register must be smaller than 32",token);
  return 0;
  

}

struct Token*nextTokenEnforceColon(struct Token*token){
  if(!token->next)
    compError("Unexpected EOF",token);
  token = token->next;
  if(token->type != Colon)
    compError("Colon expected",token);
  if(!token->next)
    compError("Unexpected EOF",token);
  token = token->next;
  return token;
}

void insert4ByteCheckLineEnd(CompContext*ctx, uint32_t enc){
  if(ctx->pass == INDEX){
    ctx->section->size += 4;
  }
  else{
    *((uint32_t*)(ctx->section->buff+ctx->section->index)) = enc;
    ctx->section->index += 4;
  }
  
  if(!ctx->token)
    return;
  if(ctx->token->type != Newline)
    compError("Line Break or EOF expected after RiscV Instruction",ctx->token);
}

/* This Function parses the nameToken of a specific relocation.
 * Pattern: %type(name)
 */
struct Token*parseRelocationPattern(CompContext*ctx,uint32_t type){
  struct Token*backupToken = ctx->token;
  if(ctx->token->type != Percent)
    goto fail;
  if(!ctx->token->next)
    goto fail;
  ctx->token = ctx->token->next;
  switch(type){
    case R_RISCV_HI20:
      if(!tokenIdentComp("hi",ctx->token))
	goto fail;
      break;
    case R_RISCV_PCREL_HI20:
      if(!tokenIdentComp("pcrel_hi",ctx->token))
	goto fail;
      break;
    default:
      goto fail;
  }
  if(!ctx->token->next)
    goto fail;
  ctx->token = ctx->token->next;
  if(ctx->token->type != BracketIn)
    goto fail;
  if(!ctx->token->next)
    goto fail;
  ctx->token = ctx->token->next;
  if(ctx->token->type != Identifier)
    goto fail;
  struct Token*nameToken = ctx->token;
  if(!ctx->token->next)
    goto fail;
  ctx->token = ctx->token->next;
  if(ctx->token->type != BracketOut)
    goto fail;
  ctx->token = ctx->token->next;
  return nameToken;
fail:
  ctx->token = backupToken;
  return NULL;
}

void encodeLui(CompContext*ctx){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  uint32_t enc = 0x37;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  if(ctx->token->type == Number){
    enc += (parseInt(ctx->token) >> 12) << 12;
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  struct Token*nameToken = parseRelocationPattern(ctx,R_RISCV_HI20);
  if(nameToken){
    addRelaEntry(ctx,ctx->section->index,getSymbolIndex(ctx,nameToken),R_RISCV_HI20,0);
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Number or \%hi() Relocation expected",ctx->token);
}

void encodeAuipc(CompContext*ctx){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  uint32_t enc = 0x17;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  if(ctx->token->type == Number){
    enc += (parseInt(ctx->token) >> 12) << 12;
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  struct Token*nameToken = parseRelocationPattern(ctx,R_RISCV_PCREL_HI20);
  if(nameToken){
    addRelaEntry(ctx,ctx->section->index,getSymbolIndex(ctx,nameToken),R_RISCV_PCREL_HI20,0);
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Number or \%pcrel_hi() Relocation expected",ctx->token);
}


void encodeR(CompContext*ctx,uint32_t enc){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 15;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 20;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32I(CompContext*ctx){
  if(tokenIdentCompCI("lui"  ,ctx->token)){encodeLui(ctx);return true;}
  if(tokenIdentCompCI("auipc",ctx->token)){encodeAuipc(ctx);return true;}
  if(tokenIdentCompCI("jal"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("jalr" ,ctx->token)){return false;}
  if(tokenIdentCompCI("beq"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("bne"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("blt"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("bge"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("bltu" ,ctx->token)){return false;}
  if(tokenIdentCompCI("bleu" ,ctx->token)){return false;}
  if(tokenIdentCompCI("lb"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("lh"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("lw"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("lbu"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("lhu"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("sb"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("sh"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("sw"   ,ctx->token)){return false;}
  if(tokenIdentCompCI("addi" ,ctx->token)){return false;}
  if(tokenIdentCompCI("slti" ,ctx->token)){return false;}
  if(tokenIdentCompCI("sltiu",ctx->token)){return false;}
  if(tokenIdentCompCI("xori" ,ctx->token)){return false;}
  if(tokenIdentCompCI("ori"  ,ctx->token)){return false;}
  if(tokenIdentCompCI("andi" ,ctx->token)){return false;}
  if(tokenIdentCompCI("slli" ,ctx->token)){return false;}
  if(tokenIdentCompCI("srli" ,ctx->token)){return false;}
  if(tokenIdentCompCI("srai" ,ctx->token)){return false;}
  if(tokenIdentCompCI("add"  ,ctx->token)){encodeR(ctx,0x00000033);return true;}
  if(tokenIdentCompCI("sub"  ,ctx->token)){encodeR(ctx,0x40000033);return true;}
  if(tokenIdentCompCI("sll"  ,ctx->token)){encodeR(ctx,0x00001033);return true;}
  if(tokenIdentCompCI("slt"  ,ctx->token)){encodeR(ctx,0x00002033);return true;}
  if(tokenIdentCompCI("sltu" ,ctx->token)){encodeR(ctx,0x00003033);return true;}
  if(tokenIdentCompCI("xor"  ,ctx->token)){encodeR(ctx,0x00004033);return true;}
  if(tokenIdentCompCI("srl"  ,ctx->token)){encodeR(ctx,0x00005033);return true;}
  if(tokenIdentCompCI("sra"  ,ctx->token)){encodeR(ctx,0x40005033);return true;}
  if(tokenIdentCompCI("or"   ,ctx->token)){encodeR(ctx,0x00006033);return true;}
  if(tokenIdentCompCI("and"  ,ctx->token)){encodeR(ctx,0x00007033);return true;}
  if(tokenIdentCompCI("fence",ctx->token)){return false;}
  if(tokenIdentCompCI("fence.tso",ctx->token)){return false;}
  if(tokenIdentCompCI("pause",ctx->token)){return false;}
  if(tokenIdentCompCI("ecall",ctx->token)){return false;}
  if(tokenIdentCompCI("ebreak",ctx->token)){return false;}
  return false;
}

bool compRV32M(CompContext*ctx){
  if(tokenIdentCompCI("mul",ctx->token))	{encodeR(ctx,0x02000033);return true;}
  if(tokenIdentCompCI("mulh",ctx->token))	{encodeR(ctx,0x02001033);return true;}
  if(tokenIdentCompCI("mulhsu",ctx->token))	{encodeR(ctx,0x02002033);return true;}
  if(tokenIdentCompCI("mulhu",ctx->token))	{encodeR(ctx,0x02003033);return true;}
  if(tokenIdentCompCI("div",ctx->token))	{encodeR(ctx,0x02004033);return true;}
  if(tokenIdentCompCI("divu",ctx->token))	{encodeR(ctx,0x02005033);return true;}
  if(tokenIdentCompCI("rem",ctx->token))	{encodeR(ctx,0x02006033);return true;}
  if(tokenIdentCompCI("remu",ctx->token))	{encodeR(ctx,0x02007033);return true;}
  return false;
}

bool compRV32A(CompContext*ctx){

  return false;
}

bool compRV32F(CompContext*ctx){

  return false;
}

bool compRV32C(CompContext*ctx){

  return false;
}

bool compRV32Zicsr(CompContext*ctx){

  return false;
}

bool compRV32Zifencei(CompContext*ctx){

  return false;
}

bool compRV(CompContext*ctx){
  if(compRV32I(ctx))
    return true;
  if(compRV32M(ctx))
    return true;
  if(compRV32A(ctx))
    return true;
  if(compRV32F(ctx))
    return true;
  if(compRV32C(ctx))
    return true;
  if(compRV32Zicsr(ctx))
    return true;
  if(compRV32Zifencei(ctx))
    return true;
  return false;
}
