
#include"riscv.h"

#include"riscv_zicsr.h"
#include"riscv_c.h"

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


/* This function parses an Int Register optionally surrounded by Brackets.
 * This function does not advance after the last relevant token.
 */
uint32_t parseBracketReg(CompContext*ctx){
  if(ctx->token->type == BracketIn){
    nextTokenEnforceExistence(ctx);
    uint32_t reg = parseIntReg(ctx->token);
    nextTokenEnforceExistence(ctx);
    if(ctx->token->type != BracketOut)
      compError(") expected",ctx->token);
    return reg;
  }
  return parseIntReg(ctx->token);
}

uint32_t parseIORW(struct Token*token){
  uint32_t res = 0;
  if(token->type == Number){
    if(parseUInt(token) == 0)
      return 0;
    else goto fail;
  }
  else if(token->type == Identifier){
    for(char*cp = token->buff; cp<token->buffTop; cp++){
      switch(*cp){
	case'i':
	case'I':
	  res |= 8;
	  break;
	case'o':
	case'O':
	  res |= 4;
	  break;
	case'r':
	case'R':
	  res |= 2;
	  break;
	case'w':
	case'W':
	  res |= 1;
	  break;
	default:
	  goto fail;
      }
    }
    return res;
  }

fail:
  compError("Expcting IORW Flags. Any combination of i,o,r,w or 0 is valid",token);
  return 0;
}

void nextTokenEnforceExistence(CompContext*ctx){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
}

void nextTokenEnforceComma(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != Comma)
    compError("Comma Expected",ctx->token);
  nextTokenEnforceExistence(ctx);
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

bool tryCompRelocation(CompContext*ctx,uint32_t type){
  struct Token*backupToken = ctx->token;
  struct Token*nameToken = NULL;
  int32_t addend = 0;
  if(ctx->token->type != Percent)
    goto fail;
  nextTokenEnforceExistence(ctx);

  if(type == R_RISCV_HI20){
    if(!tokenIdentComp("hi",ctx->token))
      goto fail;
  }else if(type == R_RISCV_LO12_I){
    if(!tokenIdentComp("lo",ctx->token))
      goto fail;
  }else if(type == R_RISCV_LO12_S){
    if(!tokenIdentComp("lo",ctx->token))
      goto fail;
  }
  else if(type == R_RISCV_PCREL_HI20){
    if(!tokenIdentComp("pcrel_hi",ctx->token))
      goto fail;
  }else if(type == R_RISCV_PCREL_LO12_I){
    if(!tokenIdentComp("pcrel_lo",ctx->token))
      goto fail;
  }else if(type == R_RISCV_PCREL_LO12_S){
    if(!tokenIdentComp("pcrel_lo",ctx->token))
      goto fail;
  }
  else if(type == R_RISCV_JAL){
    if(!tokenIdentComp("jal",ctx->token))
      goto fail;
  }else if(type == R_RISCV_BRANCH){
    if(!tokenIdentComp("branch",ctx->token))
      goto fail;
  }
  else if(type == R_RISCV_RVC_LUI){
    if(!tokenIdentComp("rvc_lui",ctx->token))
      goto fail;
  }else if(type == R_RISCV_RVC_JUMP){
    if(!tokenIdentComp("rvc_jump",ctx->token))
      goto fail;
  }else if(type == R_RISCV_RVC_BRANCH){
    if(!tokenIdentComp("rvc_branch",ctx->token))
      goto fail;
  }
  else goto fail;

  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != BracketIn)
    goto fail;
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != Identifier)
    goto fail;
  nameToken = ctx->token;
  nextTokenEnforceExistence(ctx);

  if(ctx->token->type == Number){
    addend = parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }else if(ctx->token->type == Plus){
    nextTokenEnforceExistence(ctx);
    addend = parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }else if(ctx->token->type == Minus){
    nextTokenEnforceExistence(ctx);
    addend = - parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }

  if(ctx->token->type != BracketOut)
    goto fail;
//  ctx->token = ctx->token->next;

  // Apply Relocation
  addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,nameToken),type,addend);
  return true;
fail:
  ctx->token = backupToken;
  return false;
}


void encodeLui(CompContext*ctx){
  uint32_t enc = 0x37;
  Symbol*sym;

  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number)
    enc += (parseInt(ctx->token) >> 12) << 12;

  else if(tryCompRelocation(ctx,R_RISCV_HI20));

  else if((sym = getSymbol(ctx,ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_HI20,0);

  else compError("Offset, \%hi Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeAuipc(CompContext*ctx){
  uint32_t enc = 0x17;
  Symbol*sym;

  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number)
    enc += (parseInt(ctx->token) >> 12) << 12;

  else if(tryCompRelocation(ctx,R_RISCV_PCREL_HI20));

  else if((sym=getSymbol(ctx,ctx->token)))
      addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_PCREL_HI20,0);

  else compError("Offset, \%pcrel_hi Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);

}


void encodeU(CompContext*ctx, uint32_t enc){
  Symbol*sym;

  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number){
    uint32_t offset = parseImm(ctx->token,21);
    enc += ((offset >> 1) & 0x3FF) << 21;
    enc += ((offset >> 11) & 1) << 20;
    enc += ((offset >> 12) & 0xFF) << 12;
    enc += ((offset >> 20) & 1) << 31;
  }

  else if(tryCompRelocation(ctx,R_RISCV_JAL));

  else if((sym = getSymbol(ctx,ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_JAL,0);

  else compError("Offset, \%jal Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeB(CompContext*ctx,uint32_t enc){
  Symbol*sym;

  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 15;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 20;
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number){
    uint32_t offset = parseImm(ctx->token,13);
    enc += ((offset >> 1) & 0x0F) << 8;
    enc += ((offset >> 5) & 0x3F) << 25;
    enc += ((offset >> 11) & 1) << 7;
    enc += ((offset >> 12) & 1) << 31;
  }

  else if(tryCompRelocation(ctx,R_RISCV_BRANCH));

  else if((sym = getSymbol(ctx, ctx->token)))
    addRelaEntry(ctx,ctx->section->index,sym,R_RISCV_BRANCH,0);

  else compError("Offset, \%branch Relocation or Symbol expected",ctx->token);

  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeStore(CompContext*ctx,uint32_t enc,bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token) << 20;	// rs2 
  nextTokenEnforceComma(ctx);

  if(ctx->token->type == Number){
    uint32_t imm = parseImm(ctx->token,12);
    enc += (imm & 0x1F) << 7;
    enc += (imm >> 5) << 25;
  }
  else if(tryCompRelocation(ctx,R_RISCV_LO12_S));
  else tryCompRelocation(ctx,R_RISCV_PCREL_LO12_S);
  
  nextTokenEnforceExistence(ctx);
  enc += parseBracketReg(ctx) << 15;	// rs1
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeLoadJalr(CompContext*ctx,uint32_t enc,bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token) << 7;	// rd
  nextTokenEnforceComma(ctx);
  if(ctx->token->type == Number)
    enc += parseImm(ctx->token,12) << 20;
  else if(tryCompRelocation(ctx,R_RISCV_LO12_I));
  else tryCompRelocation(ctx,R_RISCV_PCREL_LO12_I);
  
  nextTokenEnforceExistence(ctx);
  enc += parseBracketReg(ctx) << 15;	// rs1
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeR(CompContext*ctx,uint32_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 15;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 20;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeI(CompContext*ctx,uint32_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 15;
  if(ctx->token->next && ctx->token->next->type != Newline){
    nextTokenEnforceComma(ctx);
    if(ctx->token->type == Number)
      enc += parseImm(ctx->token,12) << 20;
    else if(tryCompRelocation(ctx,R_RISCV_LO12_I));
    else if(tryCompRelocation(ctx,R_RISCV_PCREL_LO12_I));
    else
      compError("The offset of an I encoding has to be a number, a \%lo(), a \%pcrel_lo() or nothing",ctx->token);
  }
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeShiftImmediate(CompContext*ctx,uint32_t enc,uint32_t shamt){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 15;
  nextTokenEnforceComma(ctx);
  enc += parseUImm(ctx->token,shamt) << 20;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeFence(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  uint32_t enc = 0x0F;
  enc += parseIORW(ctx->token) << 20;
  nextTokenEnforceComma(ctx);
  enc += parseIORW(ctx->token) << 24;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeImmediate(CompContext*ctx,uint32_t enc){
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32I(CompContext*ctx){
  if     (tokenIdentCompCI("lui"  ,ctx->token))encodeLui(ctx);
  else if(tokenIdentCompCI("auipc",ctx->token))encodeAuipc(ctx);
  else if(tokenIdentCompCI("jal"  ,ctx->token))encodeU(ctx,0x6F);
  else if(tokenIdentCompCI("jalr" ,ctx->token))encodeLoadJalr(ctx,0x67,false);
  else if(tokenIdentCompCI("beq"  ,ctx->token))encodeB(ctx,0x0063);
  else if(tokenIdentCompCI("bne"  ,ctx->token))encodeB(ctx,0x1063);
  else if(tokenIdentCompCI("blt"  ,ctx->token))encodeB(ctx,0x4063);
  else if(tokenIdentCompCI("bge"  ,ctx->token))encodeB(ctx,0x5063);
  else if(tokenIdentCompCI("bltu" ,ctx->token))encodeB(ctx,0x6063);
  else if(tokenIdentCompCI("bleu" ,ctx->token))encodeB(ctx,0x7063);
  else if(tokenIdentCompCI("lb"   ,ctx->token))encodeLoadJalr(ctx,0x0003,false);
  else if(tokenIdentCompCI("lh"   ,ctx->token))encodeLoadJalr(ctx,0x1003,false);
  else if(tokenIdentCompCI("lw"   ,ctx->token))encodeLoadJalr(ctx,0x2003,false);
  else if(tokenIdentCompCI("lbu"  ,ctx->token))encodeLoadJalr(ctx,0x4003,false);
  else if(tokenIdentCompCI("lhu"  ,ctx->token))encodeLoadJalr(ctx,0x5003,false);
  else if(tokenIdentCompCI("sb"   ,ctx->token))encodeStore(ctx,0x0023,false);
  else if(tokenIdentCompCI("sh"   ,ctx->token))encodeStore(ctx,0x1023,false);
  else if(tokenIdentCompCI("sw"   ,ctx->token))encodeStore(ctx,0x2023,false);
  else if(tokenIdentCompCI("addi" ,ctx->token))encodeI(ctx,0x0013);
  else if(tokenIdentCompCI("slti" ,ctx->token))encodeI(ctx,0x2013);
  else if(tokenIdentCompCI("sltiu",ctx->token))encodeI(ctx,0x3013);
  else if(tokenIdentCompCI("xori" ,ctx->token))encodeI(ctx,0x4013);
  else if(tokenIdentCompCI("ori"  ,ctx->token))encodeI(ctx,0x6013);
  else if(tokenIdentCompCI("andi" ,ctx->token))encodeI(ctx,0x7013);
  else if(tokenIdentCompCI("slli" ,ctx->token))encodeShiftImmediate(ctx,0x00001013,5);
  else if(tokenIdentCompCI("srli" ,ctx->token))encodeShiftImmediate(ctx,0x00005013,5);
  else if(tokenIdentCompCI("srai" ,ctx->token))encodeShiftImmediate(ctx,0x40005013,5);
  else if(tokenIdentCompCI("add"  ,ctx->token))encodeR(ctx,0x00000033);
  else if(tokenIdentCompCI("sub"  ,ctx->token))encodeR(ctx,0x40000033);
  else if(tokenIdentCompCI("sll"  ,ctx->token))encodeR(ctx,0x00001033);
  else if(tokenIdentCompCI("slt"  ,ctx->token))encodeR(ctx,0x00002033);
  else if(tokenIdentCompCI("sltu" ,ctx->token))encodeR(ctx,0x00003033);
  else if(tokenIdentCompCI("xor"  ,ctx->token))encodeR(ctx,0x00004033);
  else if(tokenIdentCompCI("srl"  ,ctx->token))encodeR(ctx,0x00005033);
  else if(tokenIdentCompCI("sra"  ,ctx->token))encodeR(ctx,0x40005033);
  else if(tokenIdentCompCI("or"   ,ctx->token))encodeR(ctx,0x00006033);
  else if(tokenIdentCompCI("and"  ,ctx->token))encodeR(ctx,0x00007033);
  else if(tokenIdentCompCI("fence",ctx->token))encodeFence(ctx);
  else if(tokenIdentCompCI("fence.tso",ctx->token))encodeImmediate(ctx,0x8330000F);
  else if(tokenIdentCompCI("pause",ctx->token))encodeImmediate(ctx,0x0100000F);
  else if(tokenIdentCompCI("ecall",ctx->token))encodeImmediate(ctx,0x00000073);
  else if(tokenIdentCompCI("ebreak",ctx->token))encodeImmediate(ctx,0x00100073);
  else return false;
  return true;
}

bool compRV32M(CompContext*ctx){
  if     (tokenIdentCompCI("mul",ctx->token))	encodeR(ctx,0x02000033);
  else if(tokenIdentCompCI("mulh",ctx->token))	encodeR(ctx,0x02001033);
  else if(tokenIdentCompCI("mulhsu",ctx->token))encodeR(ctx,0x02002033);
  else if(tokenIdentCompCI("mulhu",ctx->token))	encodeR(ctx,0x02003033);
  else if(tokenIdentCompCI("div",ctx->token))	encodeR(ctx,0x02004033);
  else if(tokenIdentCompCI("divu",ctx->token))	encodeR(ctx,0x02005033);
  else if(tokenIdentCompCI("rem",ctx->token))	encodeR(ctx,0x02006033);
  else if(tokenIdentCompCI("remu",ctx->token))	encodeR(ctx,0x02007033);
  else return false;
  return true;
}

uint32_t parseAtomicAqRl(struct Token*token,uint32_t offset){
  if(tokenIdentCompPartialCI("",token,offset))return 0;
  if(tokenIdentCompPartialCI(".aq",token,offset))return 2;
  if(tokenIdentCompPartialCI(".rl",token,offset))return 1;
  if(tokenIdentCompPartialCI(".aq.rl",token,offset))return 3;
  compError("Unknown Aq Rl Flag",token);
  return 0;
}

void encodeAtomic(CompContext*ctx,uint32_t enc,bool load,uint32_t aqrl_offset){
  enc += parseAtomicAqRl(ctx->token,aqrl_offset) << 25;
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  if(!load){
    enc += parseIntReg(ctx->token) << 20;
    nextTokenEnforceComma(ctx);
  }
  enc += parseBracketReg(ctx) << 15;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32A(CompContext*ctx){
  if     (tokenIdentCompPartialCI("lr.w",	     ctx->token,0))encodeAtomic(ctx,0x1000202F,true,4);
  else if(tokenIdentCompPartialCI("sc.w",           ctx->token,0))encodeAtomic(ctx,0x1800202F,false,4);
  else if(tokenIdentCompPartialCI("amoswap.w",	     ctx->token,0))encodeAtomic(ctx,0x0800202F,false,9);
  else if(tokenIdentCompPartialCI("amoadd.w",       ctx->token,0))encodeAtomic(ctx,0x0000202F,false,8);
  else if(tokenIdentCompPartialCI("amoxor.w",       ctx->token,0))encodeAtomic(ctx,0x2000202F,false,8);
  else if(tokenIdentCompPartialCI("amoand.w",       ctx->token,0))encodeAtomic(ctx,0x6000202F,false,8);
  else if(tokenIdentCompPartialCI("amoor.w",        ctx->token,0))encodeAtomic(ctx,0x4000202F,false,7);
  else if(tokenIdentCompPartialCI("amomin.w",       ctx->token,0))encodeAtomic(ctx,0x8000202F,false,8);
  else if(tokenIdentCompPartialCI("amomax.w",       ctx->token,0))encodeAtomic(ctx,0xA000202F,false,8);
  else if(tokenIdentCompPartialCI("amominu.w",      ctx->token,0))encodeAtomic(ctx,0xC000202F,false,9);
  else if(tokenIdentCompPartialCI("amomaxu.w",      ctx->token,0))encodeAtomic(ctx,0xE000202F,false,9);
  else return false;
  return true;
}

uint32_t parseFloatRM(struct Token*token,uint32_t offset){
  if(tokenIdentCompPartialCI(".rne",token,offset))return 0;
  if(tokenIdentCompPartialCI(".rtz",token,offset))return 1;
  if(tokenIdentCompPartialCI(".rdn",token,offset))return 2;
  if(tokenIdentCompPartialCI(".rup",token,offset))return 3;
  if(tokenIdentCompPartialCI(".rmm",token,offset))return 4;
  if(tokenIdentCompPartialCI(".dyn",token,offset))return 7;
  if(tokenIdentCompPartialCI("",token,offset))return 7;
  compError("Unknown Rounding Mode",token);
  return 0;
}

void encodeFloat4Reg(CompContext*ctx,uint32_t enc, uint32_t rm_offset){
  enc += parseFloatRM(ctx->token,rm_offset) << 12;
  nextTokenEnforceExistence(ctx);
  enc += parseFloatReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseFloatReg(ctx->token) << 15;
  nextTokenEnforceComma(ctx);
  enc += parseFloatReg(ctx->token) << 20;
  nextTokenEnforceComma(ctx);
  enc += parseFloatReg(ctx->token) << 27;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeFloatReg(CompContext*ctx,uint32_t enc, uint32_t rm_offset, bool rm, bool rs2){
  if(rm)
    enc += parseFloatRM(ctx->token,rm_offset) << 12;
  nextTokenEnforceExistence(ctx);
  enc += parseFloatReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseFloatReg(ctx->token) << 15;
  if(rs2){
    nextTokenEnforceComma(ctx);
    enc += parseFloatReg(ctx->token) << 20;
  }
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32F(CompContext*ctx){
  if     (tokenIdentCompCI("flw",ctx->token))encodeLoadJalr(ctx,0x2007,true);
  else if(tokenIdentCompCI("fsw",ctx->token))encodeStore(ctx,0x2027,true);
  else if(tokenIdentCompPartialCI("fmadd.s",ctx->token,0))encodeFloat4Reg(ctx,0x83,7);
  else if(tokenIdentCompPartialCI("fmsub.s",ctx->token,0))encodeFloat4Reg(ctx,0x87,7);
  else if(tokenIdentCompPartialCI("fnmsub.s",ctx->token,0))encodeFloat4Reg(ctx,0x8B,8);
  else if(tokenIdentCompPartialCI("fnmadd.s",ctx->token,0))encodeFloat4Reg(ctx,0x8F,8);
  else if(tokenIdentCompPartialCI("fadd.s",ctx->token,0))encodeFloatReg(ctx,0x00000053,6,true,true);
  else if(tokenIdentCompPartialCI("fsub.s",ctx->token,0))encodeFloatReg(ctx,0x08000053,6,true,true);
  else if(tokenIdentCompPartialCI("fmul.s",ctx->token,0))encodeFloatReg(ctx,0x10000053,6,true,true);
  else if(tokenIdentCompPartialCI("fdiv.s",ctx->token,0))encodeFloatReg(ctx,0x18000053,6,true,true);
  else if(tokenIdentCompPartialCI("fsqrt.s",ctx->token,0))encodeFloatReg(ctx,0x58000053,7,true,false);
  else if(tokenIdentCompCI("fsgnj.s",ctx->token))encodeFloatReg(ctx,0x20000053,0,false,true);
  else if(tokenIdentCompCI("fsgnjn.s",ctx->token))encodeFloatReg(ctx,0x20001053,0,false,true);
  else if(tokenIdentCompCI("fsgnjx.s",ctx->token))encodeFloatReg(ctx,0x20002053,0,false,true);
  else if(tokenIdentCompCI("fmin.s",ctx->token))encodeFloatReg(ctx,0x28000053,0,false,true);
  else if(tokenIdentCompCI("fmax.s",ctx->token))encodeFloatReg(ctx,0x28001053,0,false,true);
  else if(tokenIdentCompPartialCI("fcvt.w.s",ctx->token,0))encodeFloatReg(ctx,0xC0000053,8,true,false);
  else if(tokenIdentCompPartialCI("fcvt.wu.s",ctx->token,0))encodeFloatReg(ctx,0xC0100053,9,true,false);
  else if(tokenIdentCompCI("fmv.x.w",ctx->token))encodeFloatReg(ctx,0xE0000053,0,false,false);
  else if(tokenIdentCompCI("feq.s",ctx->token))encodeFloatReg(ctx,0xA0002053,0,false,true);
  else if(tokenIdentCompCI("flt.s",ctx->token))encodeFloatReg(ctx,0xA0001053,0,false,true);
  else if(tokenIdentCompCI("fle.s",ctx->token))encodeFloatReg(ctx,0xA0000053,0,false,true);
  else if(tokenIdentCompCI("fclass.s",ctx->token))encodeFloatReg(ctx,0xE0001053,0,false,false);
  else if(tokenIdentCompPartialCI("fcvt.s.w",ctx->token,0))encodeFloatReg(ctx,0xD0000053,8,true,false);
  else if(tokenIdentCompPartialCI("fcvt.s.wu",ctx->token,0))encodeFloatReg(ctx,0xD0100053,9,true,false);
  else if(tokenIdentComp("fmv.w.x",ctx->token))encodeFloatReg(ctx,0xF0000053,0,false,false);
  else return false;
  return true;
}


bool compRV32Zifencei(CompContext*ctx){
  if(tokenIdentCompCI("fence.i",ctx->token)){
    encodeImmediate(ctx,0x0000100F);
    return true;
  }
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
