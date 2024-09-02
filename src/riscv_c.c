
#include"riscv_c.h"
#include"riscv.h"

#include"token.h"
#include"relocation.h"
#include"symbol.h"

uint32_t parseRvcIntReg(struct Token*token){
  if(tokenIdentCompCI("x8",token))return 0;
  if(tokenIdentCompCI("x9",token))return 1;
  if(tokenIdentCompCI("x10",token))return 2;
  if(tokenIdentCompCI("x11",token))return 3;
  if(tokenIdentCompCI("x12",token))return 4;
  if(tokenIdentCompCI("x13",token))return 5;
  if(tokenIdentCompCI("x14",token))return 6;
  if(tokenIdentCompCI("x15",token))return 7;
  if(tokenIdentCompCI("s0",token))return 0;
  if(tokenIdentCompCI("s1",token))return 1;
  if(tokenIdentCompCI("a0",token))return 2;
  if(tokenIdentCompCI("a1",token))return 3;
  if(tokenIdentCompCI("a2",token))return 4;
  if(tokenIdentCompCI("a3",token))return 5;
  if(tokenIdentCompCI("a4",token))return 6;
  if(tokenIdentCompCI("a5",token))return 7;
  compError("Unknown RVC Int Register",token);
  return 0;
}

uint32_t parseRvcFloatReg(struct Token*token){
  if(tokenIdentCompCI("f8",token))return 0;
  if(tokenIdentCompCI("f9",token))return 1;
  if(tokenIdentCompCI("f10",token))return 2;
  if(tokenIdentCompCI("f11",token))return 3;
  if(tokenIdentCompCI("f12",token))return 4;
  if(tokenIdentCompCI("f13",token))return 5;
  if(tokenIdentCompCI("f14",token))return 6;
  if(tokenIdentCompCI("f15",token))return 7;
  if(tokenIdentCompCI("fs0",token))return 0;
  if(tokenIdentCompCI("fs1",token))return 1;
  if(tokenIdentCompCI("fa0",token))return 2;
  if(tokenIdentCompCI("fa1",token))return 3;
  if(tokenIdentCompCI("fa2",token))return 4;
  if(tokenIdentCompCI("fa3",token))return 5;
  if(tokenIdentCompCI("fa4",token))return 6;
  if(tokenIdentCompCI("fa5",token))return 7;
  compError("Unknown RVC Float Register",token);
  return 0;
}

uint32_t parseRvcBracketReg(CompContext*ctx){
  if(ctx->token->type == BracketIn){
    nextTokenEnforceExistence(ctx);
    uint32_t reg = parseRvcIntReg(ctx->token);
    nextTokenEnforceExistence(ctx);
    if(ctx->token->type != BracketOut)
      compError(") Expected",ctx->token);
    return reg;
  }
  return parseRvcIntReg(ctx->token);
}

void insert2ByteCheckLineEnd(CompContext*ctx, uint16_t enc){
  if(ctx->pass == INDEX){
    ctx->section->size += 2;
  }
  else{
    *((uint16_t*)(ctx->section->buff+ctx->section->index)) = enc;
    ctx->section->index += 2;
  } 
  if(!ctx->token)
    return;
  if(ctx->token->type != Newline)
    compError("Line Break or EOF expected after RiscV Instruction",ctx->token);
}


void encodeRvcCiLoadSp(CompContext*ctx, uint16_t enc, uint32_t imm_shift, bool float_reg){
  nextTokenEnforceExistence(ctx);
  uint32_t n = float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token);
  if(!float_reg && n==0)
    compError("rd must not be 0",ctx->token);
  enc += n << 2;
  nextTokenEnforceComma(ctx);
  n = parseImm(ctx->token,6 + imm_shift);
  if(imm_shift == 2){
    if((n&3)!=0)compError("Offset must be divisible by 4",ctx->token);
    enc += ((n>>6)&3)<<2;
    enc += ((n>>2)&7)<<4;
  }else if(imm_shift == 3){
    if((n&7)!=0)compError("Offset must be divisible by 8",ctx->token);
    enc += ((n>>6)&7)<<2;
    enc += ((n>>3)&3)<<5;
  }else if(imm_shift == 4){
    if((n&15)!=0)compError("Offset must be divisible by 16",ctx->token);
    enc += ((n>>6)&15)<<2;
    enc += ((n>>4)&1)<<6;
  }
  enc += ((n>>5)&1)<<12;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCss(CompContext*ctx, uint16_t enc, uint32_t imm_shift, bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += (float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token)) << 2;
  nextTokenEnforceComma(ctx);
  uint32_t n = parseImm(ctx->token,6 + imm_shift);
  if(imm_shift == 2){
    if((n&3)!=0)compError("Offset must be divisible by 4",ctx->token);
    enc += ((n>>6)&3) << 7;
    enc += ((n>>2)&15) << 9;
  }else if(imm_shift == 3){
    if((n&7)!=0)compError("Offset must be divisible by 8",ctx->token);
    enc += ((n>>6)&7) << 7;
    enc += ((n>>3)&7) << 10;
  }else if(imm_shift == 4){
    if((n&15)!=0)compError("Offset must be divisible by 16",ctx->token);
    enc += ((n>>7)&15) << 7;
    enc += ((n>>4)&3) << 11;
  }
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcClCs(CompContext*ctx, uint16_t enc, uint32_t imm_shift, bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += (float_reg ? parseRvcFloatReg(ctx->token) : parseRvcIntReg(ctx->token)) << 2;
  nextTokenEnforceComma(ctx);
  uint32_t n = parseImm(ctx->token, 5 + imm_shift);
  if(imm_shift == 2){
    if((n&3)!=0)compError("Offset must be divisible by 4",ctx->token);
    enc += ((n>>6)&1)<<5;
    enc += ((n>>2)&1)<<6;
    enc += ((n>>3)&7)<<10;
  }else if(imm_shift == 3){
    if((n&7)!=0)compError("Offset must be divisible by 8",ctx->token);
    enc += ((n>>6)&3)<<5;
    enc += ((n>>3)&7)<<10;
  }else if(imm_shift == 4){
    if((n&15)!=0)compError("Offset must be divisible by 16",ctx->token);
    enc += ((n>>6)&3)<<5;
    enc += ((n>>8)&1)<<10;
    enc += ((n>>4)&3)<<11;
  }
  nextTokenEnforceExistence(ctx);
  enc += parseBracketReg(ctx) << 7;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCj(CompContext*ctx, uint16_t enc){
  Symbol*sym;
  nextTokenEnforceExistence(ctx);

  if(ctx->token->type == Number){
    uint32_t n = parseImm(ctx->token,12);
    if((n&1)!=0)compError("Jump Offset must be divisible by 2",ctx->token);
    enc += ((n>>5)&1)<<2;
    enc += ((n>>1)&7)<<3;
    enc += ((n>>7)&1)<<6;
    enc += ((n>>6)&1)<<7;
    enc += ((n>>10)&1)<<8;
    enc += ((n>>8)&3)<<9;
    enc += ((n>>4)&1)<<11;
    enc += ((n>>11)&1)<<12;
  }

  else if(tryCompRelocation(ctx,R_RISCV_RVC_JUMP));

  else if((sym = getSymbol(ctx, ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_RVC_JUMP,0);

  else compError("Offset, Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

// Jump Register
void encodeRvcCrJump(CompContext*ctx,uint16_t enc){
  nextTokenEnforceExistence(ctx);
  uint32_t reg = parseIntReg(ctx->token);
  if(reg==0)compError("Register must not be zero",ctx->token);
  enc += reg << 7;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCb(CompContext*ctx,uint16_t enc){
  Symbol*sym;

  nextTokenEnforceExistence(ctx);
  enc += parseRvcIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number){
    uint32_t n = parseImm(ctx->token,9);
    if((n&1)!=0)compError("Offset must be divisible by 2",ctx->token);
    enc += ((n>>5)&1)<<2;
    enc += ((n>>1)&3)<<3;
    enc += ((n>>6)&3)<<5;
    enc += ((n>>3)&3)<<10;
    enc += ((n>>8)&1)<<12;
  }

  else if(tryCompRelocation(ctx,R_RISCV_RVC_BRANCH));

  else if((sym = getSymbol(ctx,ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_RVC_BRANCH,0);
  
  else compError("Offset, Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcLi(CompContext*ctx){
  uint16_t enc = 0x4001;
  nextTokenEnforceExistence(ctx);
  uint32_t n = parseIntReg(ctx->token);
  if(n==0)compError("Register must not be zero",ctx->token);
  enc += n << 7;
  nextTokenEnforceComma(ctx);
  n = parseImm(ctx->token,6);
  enc += (n&31)<<2;
  enc += ((n>>5)&1)<<12;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcLui(CompContext*ctx){
  uint16_t enc = 0x6001;
  Symbol*sym;
  nextTokenEnforceExistence(ctx);
  uint32_t n = parseIntReg(ctx->token);
  if(n==0||n==2)compError("Register must not be 0 or 2",ctx->token);
  enc += n << 7;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number){
    uint32_t n = parseImm(ctx->token, 18) >> 12;
    if(n==0)compError("Upper Immediate must not be 0",ctx->token);
    enc += (n&31)<<2;
    enc += ((n>>5)&1)<<12;
  }

  else if(tryCompRelocation(ctx,R_RISCV_RVC_LUI));

  else if((sym = getSymbol(ctx,ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_RVC_LUI,0);

  else compError("Offset, Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCi(CompContext*ctx,uint16_t enc, bool imm_zero){
  nextTokenEnforceExistence(ctx);
  uint32_t n = parseIntReg(ctx->token);
  if(n==0)compError("Register must not be 0",ctx->token);
  enc += n << 7;
  nextTokenEnforceComma(ctx);

  n = parseImm(ctx->token,6);
  if(!imm_zero && n==0)
    compError("Immediate Value must not be 0",ctx->token);
  enc += (n&31)<<2;
  enc += ((n>>5)&1)<<12;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcAddi16sp(CompContext*ctx){
  uint16_t enc = 0x6101;
  nextTokenEnforceExistence(ctx);
  uint32_t n = parseImm(ctx->token,10);
  if((n&15)!=0)compError("Immediate Value must be divisible by 16",ctx->token);
  enc += ((n>>5)&1)<<2;
  enc += ((n>>7)&3)<<3;
  enc += ((n>>6)&1)<<5;
  enc += ((n>>4)&1)<<6;
  enc += ((n>>9)&1)<<12;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

// Used for C.ADDI4SPN
void encodeRvcAddi4spn(CompContext*ctx){
  uint16_t enc = 0x0000;
  nextTokenEnforceExistence(ctx);
  enc += parseRvcIntReg(ctx->token) << 2;
  nextTokenEnforceComma(ctx);
  uint32_t uimm = parseUImm(ctx->token,10);
  if(uimm==0 || (uimm&3)!=0)
    compError("Immediate value must be divisible by 4 and not be 0",ctx->token);
  enc += ((uimm>>3)&1)<<5;
  enc += ((uimm>>2)&1)<<6;
  enc += ((uimm>>6)&15)<<7;
  enc += ((uimm>>4)&3)<<11;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCbMisc(CompContext*ctx, uint16_t enc, bool imm_signed){
  nextTokenEnforceExistence(ctx);
  enc += parseRvcIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  uint32_t n = imm_signed ? parseImm(ctx->token,6) : parseUImm(ctx->token,6);
  enc += (n&31)<<2;
  enc += ((n>>5)&1)<<13;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCr(CompContext*ctx,uint16_t enc){
  nextTokenEnforceExistence(ctx);
  uint32_t reg = parseIntReg(ctx->token);
  if(reg==0)compError("Register must not be zero",ctx->token);
  enc += reg << 7;
  nextTokenEnforceComma(ctx);
  reg = parseIntReg(ctx->token);
  if(reg==0)compError("Register must not be zero",ctx->token);
  enc += reg << 2;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

void encodeRvcCa(CompContext*ctx,uint16_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseRvcIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseRvcIntReg(ctx->token) << 2;
  ctx->token = ctx->token->next;
  insert2ByteCheckLineEnd(ctx,enc);
}

bool compRV32C(CompContext*ctx){
  if     (tokenIdentCompCI("c.lwsp",ctx->token))encodeRvcCiLoadSp(ctx,0x4002,2,false);
  else if(tokenIdentCompCI("c.ldsp",ctx->token))encodeRvcCiLoadSp(ctx,0x6002,3,false);
  else if(tokenIdentCompCI("c.lqsp",ctx->token))encodeRvcCiLoadSp(ctx,0x2002,4,false);
  else if(tokenIdentCompCI("c.flwsp",ctx->token))encodeRvcCiLoadSp(ctx,0x6002,2,true);
  else if(tokenIdentCompCI("c.fldsp",ctx->token))encodeRvcCiLoadSp(ctx,0x2002,3,true);

  else if(tokenIdentCompCI("c.swsp",ctx->token))encodeRvcCss(ctx,0xC002,2,false);
  else if(tokenIdentCompCI("c.sdsp",ctx->token))encodeRvcCss(ctx,0xE002,3,false);
  else if(tokenIdentCompCI("c.sqsp",ctx->token))encodeRvcCss(ctx,0xA002,4,false);
  else if(tokenIdentCompCI("c.fswsp",ctx->token))encodeRvcCss(ctx,0xE002,2,true);
  else if(tokenIdentCompCI("c.fsdsp",ctx->token))encodeRvcCss(ctx,0xA002,3,true);

  else if(tokenIdentCompCI("c.lw",ctx->token))encodeRvcClCs(ctx,0x4000,2,false);
  else if(tokenIdentCompCI("c.ld",ctx->token))encodeRvcClCs(ctx,0x6000,3,false);
  else if(tokenIdentCompCI("c.lq",ctx->token))encodeRvcClCs(ctx,0x2000,4,false);
  else if(tokenIdentCompCI("c.flw",ctx->token))encodeRvcClCs(ctx,0x6000,2,true);
  else if(tokenIdentCompCI("c.fld",ctx->token))encodeRvcClCs(ctx,0x2000,3,true);
  else if(tokenIdentCompCI("c.sw",ctx->token))encodeRvcClCs(ctx,0xC000,2,false);
  else if(tokenIdentCompCI("c.sd",ctx->token))encodeRvcClCs(ctx,0xE000,3,false);
  else if(tokenIdentCompCI("c.sq",ctx->token))encodeRvcClCs(ctx,0xA000,4,false);
  else if(tokenIdentCompCI("c.fsw",ctx->token))encodeRvcClCs(ctx,0xE000,2,true);
  else if(tokenIdentCompCI("c.fsd",ctx->token))encodeRvcClCs(ctx,0xA000,3,true);

  else if(tokenIdentCompCI("c.j",ctx->token))encodeRvcCj(ctx,0xA001);
  else if(tokenIdentCompCI("c.jal",ctx->token))encodeRvcCj(ctx,0x2001);

  else if(tokenIdentCompCI("c.jr",ctx->token))encodeRvcCrJump(ctx,0x8002);
  else if(tokenIdentCompCI("c.jalr",ctx->token))encodeRvcCrJump(ctx,0x9002);

  else if(tokenIdentCompCI("c.beqz",ctx->token))encodeRvcCb(ctx,0xC001);
  else if(tokenIdentCompCI("c.bnez",ctx->token))encodeRvcCb(ctx,0xE001);

  else if(tokenIdentCompCI("c.li",ctx->token))encodeRvcLi(ctx);
  else if(tokenIdentCompCI("c.lui",ctx->token))encodeRvcLui(ctx);

  else if(tokenIdentCompCI("c.addi",ctx->token))encodeRvcCi(ctx,0x0001,false);
  else if(tokenIdentCompCI("c.addiw",ctx->token))encodeRvcCi(ctx,0x2001,true);

  else if(tokenIdentCompCI("c.addi16sp",ctx->token))encodeRvcAddi16sp(ctx);
  else if(tokenIdentCompCI("c.addi4spn",ctx->token))encodeRvcAddi4spn(ctx);

  else if(tokenIdentCompCI("c.slli",ctx->token))encodeRvcCi(ctx,0x0002,true);

  else if(tokenIdentCompCI("c.srli",ctx->token))encodeRvcCbMisc(ctx,0x8001,false);
  else if(tokenIdentCompCI("c.srai",ctx->token))encodeRvcCbMisc(ctx,0x8401,false);
  else if(tokenIdentCompCI("c.andi",ctx->token))encodeRvcCbMisc(ctx,0x8801,true);

  else if(tokenIdentCompCI("c.mv",ctx->token))encodeRvcCr(ctx,0x8002);
  else if(tokenIdentCompCI("c.add",ctx->token))encodeRvcCr(ctx,0x9002);

  // CA Format
  else if(tokenIdentCompCI("c.and",ctx->token))encodeRvcCa(ctx,0x8C61);
  else if(tokenIdentCompCI("c.or",ctx->token))encodeRvcCa(ctx,0x8C41);
  else if(tokenIdentCompCI("c.xor",ctx->token))encodeRvcCa(ctx,0x8C21);
  else if(tokenIdentCompCI("c.sub",ctx->token))encodeRvcCa(ctx,0x8C01);
  else if(tokenIdentCompCI("c.addw",ctx->token))encodeRvcCa(ctx,0x9C21);
  else if(tokenIdentCompCI("c.subw",ctx->token))encodeRvcCa(ctx,0x0C01);

  else return false;
  return true;
}

