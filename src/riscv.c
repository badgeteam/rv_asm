
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

/* This function parses an Int Register optionally surrounded by Brackets.
 * This function does not advance after the last relevant token.
 */
uint32_t parseBracketReg(CompContext*ctx){
  if(ctx->token->type == BracketIn){
    if(!ctx->token->next)
      compError("Unexpected EOF",ctx->token);
    ctx->token = ctx->token->next;
    uint32_t reg = parseIntReg(ctx->token);
    if(!ctx->token->next || ctx->token->next->type != BracketOut)
      compError("Expecting )",ctx->token);
    ctx->token = ctx->token->next;
    return reg;
  }
  return parseIntReg(ctx->token);
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
  }else if(type == R_RISCV_PCREL_HI20){
    if(!tokenIdentComp("pcrel_hi",ctx->token))
      goto fail;
  }else if(type == R_RISCV_PCREL_LO12_I){
    if(!tokenIdentComp("pcrel_lo",ctx->token))
      goto fail;
  }else if(type == R_RISCV_PCREL_LO12_S){
    if(!tokenIdentComp("pcrel_lo",ctx->token))
      goto fail;
  }else goto fail;

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
  ctx->token = ctx->token->next;

  // Apply Relocation
  addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,nameToken),type,addend);
  return true;
fail:
  ctx->token = backupToken;
  return false;
}

void encodeLui(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  uint32_t enc = 0x37;
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  if(ctx->token->type == Number){
    enc += (parseInt(ctx->token) >> 12) << 12;
    ctx->token = ctx->token->next;
  }
  else if(!tryCompRelocation(ctx,R_RISCV_HI20))
    compError("Number or \%hi relocation expected",ctx->token);

  insert4ByteCheckLineEnd(ctx,enc);
  return;
}

void encodeAuipc(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  uint32_t enc = 0x17;
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  if(ctx->token->type == Number){
    enc += (parseInt(ctx->token) >> 12) << 12;
    ctx->token = ctx->token->next;
  }
  else if(!tryCompRelocation(ctx,R_RISCV_PCREL_HI20))
    compError("Number or \%pcrel_hi relocation expected",ctx->token);

  insert4ByteCheckLineEnd(ctx,enc);
  return;
}

void encodeU(CompContext*ctx, uint32_t enc){
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
  if(ctx->token->type == Identifier){
    addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,ctx->token),R_RISCV_JAL,0); 
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Symbol Name or Number expected",ctx->token);
}

void encodeB(CompContext*ctx,uint32_t enc){
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
  if(ctx->token->type == Identifier){
    addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,ctx->token),R_RISCV_BRANCH,0);
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Symbol Name or Number expected",ctx->token);
}

void encodeStore(CompContext*ctx,uint32_t enc,bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token) << 20;	// rs2 
  nextTokenEnforceComma(ctx);
  if(ctx->token->type == Number){
    uint32_t imm = parseImm(ctx->token,12);
    enc += (imm & 0x1F) << 7;
    enc += (imm >> 5) << 25;
    nextTokenEnforceExistence(ctx);
  }
  else if(tryCompRelocation(ctx,R_RISCV_LO12_S));
  else tryCompRelocation(ctx,R_RISCV_PCREL_LO12_S);
  enc += parseBracketReg(ctx) << 15;	// rs1
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeLoadJalr(CompContext*ctx,uint32_t enc,bool float_reg){
  nextTokenEnforceExistence(ctx);
  enc += float_reg ? parseFloatReg(ctx->token) : parseIntReg(ctx->token) << 7;	// rd
  nextTokenEnforceComma(ctx);
  if(ctx->token->type == Number){
    enc += parseImm(ctx->token,12) << 20;
    nextTokenEnforceExistence(ctx);
  }
  else if(tryCompRelocation(ctx,R_RISCV_LO12_I));
  else tryCompRelocation(ctx,R_RISCV_PCREL_LO12_I);
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
    if(ctx->token->type == Number){
      enc += parseImm(ctx->token,12) << 20;
      ctx->token = ctx->token->next;
    }
    else if(tryCompRelocation(ctx,R_RISCV_LO12_I));
    else if(tryCompRelocation(ctx,R_RISCV_PCREL_LO12_I));
    else
      compError("The offset of an I encoding has to be a number, a \%lo(), a \%pcrel_lo() or nothing",ctx->token);
  }else{
    ctx->token = ctx->token->next;
  }
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

bool compRV32C(CompContext*ctx){

  return false;
}

uint32_t parseCsr(struct Token*token){
  // Unprivileged Floating Point CSRs
  if(tokenIdentCompCI("fflags",token))return 0x001;
  if(tokenIdentCompCI("frm",token))return 0x002;
  if(tokenIdentCompCI("fcsr",token))return 0x003;
  // Unprivilleged Counters/Timers
  if(tokenIdentCompCI("cycle",token))return 0xC00;
  if(tokenIdentCompCI("time",token))return 0xC01;
  if(tokenIdentCompCI("instret",token))return 0xC02;
  if(tokenIdentCompCI("hpmcounter3",token))return 0xC03;
  if(tokenIdentCompCI("hpmcounter4",token))return 0xC04;
  if(tokenIdentCompCI("hpmcounter5",token))return 0xC05;
  if(tokenIdentCompCI("hpmcounter6",token))return 0xC06;
  if(tokenIdentCompCI("hpmcounter7",token))return 0xC07;
  if(tokenIdentCompCI("hpmcounter8",token))return 0xC08;
  if(tokenIdentCompCI("hpmcounter9",token))return 0xC09;
  if(tokenIdentCompCI("hpmcounter10",token))return 0xC0A;
  if(tokenIdentCompCI("hpmcounter11",token))return 0xC0B;
  if(tokenIdentCompCI("hpmcounter12",token))return 0xC0C;
  if(tokenIdentCompCI("hpmcounter13",token))return 0xC0D;
  if(tokenIdentCompCI("hpmcounter14",token))return 0xC0E;
  if(tokenIdentCompCI("hpmcounter15",token))return 0xC0F;
  if(tokenIdentCompCI("hpmcounter16",token))return 0xC10;
  if(tokenIdentCompCI("hpmcounter17",token))return 0xC11;
  if(tokenIdentCompCI("hpmcounter18",token))return 0xC12;
  if(tokenIdentCompCI("hpmcounter19",token))return 0xC13;
  if(tokenIdentCompCI("hpmcounter20",token))return 0xC14;
  if(tokenIdentCompCI("hpmcounter21",token))return 0xC15;
  if(tokenIdentCompCI("hpmcounter22",token))return 0xC16;
  if(tokenIdentCompCI("hpmcounter23",token))return 0xC17;
  if(tokenIdentCompCI("hpmcounter24",token))return 0xC18;
  if(tokenIdentCompCI("hpmcounter25",token))return 0xC19;
  if(tokenIdentCompCI("hpmcounter26",token))return 0xC1A;
  if(tokenIdentCompCI("hpmcounter27",token))return 0xC1B;
  if(tokenIdentCompCI("hpmcounter28",token))return 0xC1C;
  if(tokenIdentCompCI("hpmcounter29",token))return 0xC1D;
  if(tokenIdentCompCI("hpmcounter30",token))return 0xC1E;
  if(tokenIdentCompCI("hpmcounter31",token))return 0xC1F;
  if(tokenIdentCompCI("cycleh",token))return 0xC80;
  if(tokenIdentCompCI("timeh",token))return 0xC81;
  if(tokenIdentCompCI("instreth",token))return 0xC82;
  if(tokenIdentCompCI("hpmcounter3h",token))return 0xC83;
  if(tokenIdentCompCI("hpmcounter4h",token))return 0xC84;
  if(tokenIdentCompCI("hpmcounter5h",token))return 0xC85;
  if(tokenIdentCompCI("hpmcounter6h",token))return 0xC86;
  if(tokenIdentCompCI("hpmcounter7h",token))return 0xC87;
  if(tokenIdentCompCI("hpmcounter8h",token))return 0xC88;
  if(tokenIdentCompCI("hpmcounter9h",token))return 0xC89;
  if(tokenIdentCompCI("hpmcounter10h",token))return 0xC8A;
  if(tokenIdentCompCI("hpmcounter11h",token))return 0xC8B;
  if(tokenIdentCompCI("hpmcounter12h",token))return 0xC8C;
  if(tokenIdentCompCI("hpmcounter13h",token))return 0xC8D;
  if(tokenIdentCompCI("hpmcounter14h",token))return 0xC8E;
  if(tokenIdentCompCI("hpmcounter15h",token))return 0xC8F;
  if(tokenIdentCompCI("hpmcounter16h",token))return 0xC90;
  if(tokenIdentCompCI("hpmcounter17h",token))return 0xC91;
  if(tokenIdentCompCI("hpmcounter18h",token))return 0xC92;
  if(tokenIdentCompCI("hpmcounter19h",token))return 0xC93;
  if(tokenIdentCompCI("hpmcounterh20",token))return 0xC94;
  if(tokenIdentCompCI("hpmcounterh21",token))return 0xC95;
  if(tokenIdentCompCI("hpmcounterh22",token))return 0xC96;
  if(tokenIdentCompCI("hpmcounterh23",token))return 0xC97;
  if(tokenIdentCompCI("hpmcounterh24",token))return 0xC98;
  if(tokenIdentCompCI("hpmcounterh25",token))return 0xC99;
  if(tokenIdentCompCI("hpmcounterh26",token))return 0xC9A;
  if(tokenIdentCompCI("hpmcounterh27",token))return 0xC9B;
  if(tokenIdentCompCI("hpmcounterh28",token))return 0xC9C;
  if(tokenIdentCompCI("hpmcounterh29",token))return 0xC9D;
  if(tokenIdentCompCI("hpmcounter30h",token))return 0xC9E;
  if(tokenIdentCompCI("hpmcounter31h",token))return 0xC9F;
  // Supervisor Trap Setup
  if(tokenIdentCompCI("sstatus",token))return 0x100;
  if(tokenIdentCompCI("sie",token))return 0x104;
  if(tokenIdentCompCI("stvec",token))return 0x105;
  if(tokenIdentCompCI("scounteren",token))return 0x106;
  // Supervisor Configuration
  if(tokenIdentCompCI("senvcfg",token))return 0x10A;
  // Supervisor Counter Setup
  if(tokenIdentCompCI("scountinhibit",token))return 0x120;
  // Supervisor Trap Handling
  if(tokenIdentCompCI("sscratch",token))return 0x140;
  if(tokenIdentCompCI("sepc",token))return 0x141;
  if(tokenIdentCompCI("scause",token))return 0x142;
  if(tokenIdentCompCI("stval",token))return 0x143;
  if(tokenIdentCompCI("sip",token))return 0x144;
  if(tokenIdentCompCI("scountovf",token))return 0xDA0;
  // Supervisor Protection and Translation
  if(tokenIdentCompCI("satp",token))return 0x180;
  // Debug/Trace Registers
  if(tokenIdentCompCI("scontext",token))return 0x5A8;
  // Supervisor State Enable Registers
  if(tokenIdentCompCI("sstateen0",token))return 0x10C;
  if(tokenIdentCompCI("sstateen1",token))return 0x10D;
  if(tokenIdentCompCI("sstateen2",token))return 0x10E;
  if(tokenIdentCompCI("sstateen3",token))return 0x10F;
  // Hypervisor Trap Setup
  if(tokenIdentCompCI("hstatus",token))return 0x600;
  if(tokenIdentCompCI("hedeleg",token))return 0x602;
  if(tokenIdentCompCI("hideleg",token))return 0x603;
  if(tokenIdentCompCI("hie",token))return 0x604;
  if(tokenIdentCompCI("hcounteren",token))return 0x606;
  if(tokenIdentCompCI("hgeie",token))return 0x607;
  if(tokenIdentCompCI("hedelegh",token))return 0x612;
  // Hypervisor Trap Handling
  if(tokenIdentCompCI("htval",token))return 0x643;
  if(tokenIdentCompCI("hip",token))return 0x644;
  if(tokenIdentCompCI("hvip",token))return 0x645;
  if(tokenIdentCompCI("htinst",token))return 0x64A;
  if(tokenIdentCompCI("hgeip",token))return 0xE12;
  // Hypervisor Configuration
  if(tokenIdentCompCI("henvcfg",token))return 0x60A;
  if(tokenIdentCompCI("henvcfgh",token))return 0x61A;
  // Hypervisor Protection and Translation
  if(tokenIdentCompCI("hgatp",token))return 0x680;
  // Debug/Trace Registers
  if(tokenIdentCompCI("hcontext",token))return 0x6A8;
  // Hypervisor Counter/Timer Virtualization Registers
  if(tokenIdentCompCI("htimedelta",token))return 0x605;
  if(tokenIdentCompCI("htimedeltah",token))return 0x615;
  // Hypervisor State Enable Registers
  if(tokenIdentCompCI("hstateen0",token))return 0x60C;
  if(tokenIdentCompCI("hstateen1",token))return 0x60D;
  if(tokenIdentCompCI("hstateen2",token))return 0x60E;
  if(tokenIdentCompCI("hstateen3",token))return 0x60F;
  if(tokenIdentCompCI("hstateen0h",token))return 0x61C;
  if(tokenIdentCompCI("hstateen1h",token))return 0x61D;
  if(tokenIdentCompCI("hstateen2h",token))return 0x61E;
  if(tokenIdentCompCI("hstateen3h",token))return 0x61F;
  // Virtual Supervisor Registers
  if(tokenIdentCompCI("vsstatus",token))return 0x200;
  if(tokenIdentCompCI("vsie",token))return 0x204;
  if(tokenIdentCompCI("vstvec",token))return 0x205;
  if(tokenIdentCompCI("vsscratch",token))return 0x240;
  if(tokenIdentCompCI("vsepc",token))return 0x241;
  if(tokenIdentCompCI("vscause",token))return 0x242;
  if(tokenIdentCompCI("vstval",token))return 0x243;
  if(tokenIdentCompCI("vsip",token))return 0x244;
  if(tokenIdentCompCI("vsatp",token))return 0x280;
  // Machine Information Registers
  if(tokenIdentCompCI("mvendorid",token))return 0xF11;
  if(tokenIdentCompCI("marchid",token))return 0xF12;
  if(tokenIdentCompCI("mimpid",token))return 0xF13;
  if(tokenIdentCompCI("mhartid",token))return 0xF14;
  if(tokenIdentCompCI("mconfigptr",token))return 0xF15;
  // Machine Trap Setup
  if(tokenIdentCompCI("mstatus",token))return 0x300;
  if(tokenIdentCompCI("misa",token))return 0x301;
  if(tokenIdentCompCI("medeleg",token))return 0x302;
  if(tokenIdentCompCI("mideleg",token))return 0x303;
  if(tokenIdentCompCI("mie",token))return 0x304;
  if(tokenIdentCompCI("mtvec",token))return 0x305;
  if(tokenIdentCompCI("mcounteren",token))return 0x306;
  if(tokenIdentCompCI("mstatush",token))return 0x310;
  if(tokenIdentCompCI("medelegh",token))return 0x312;
  // Machine Trap Handling
  if(tokenIdentCompCI("mscratch",token))return 0x340;
  if(tokenIdentCompCI("mepc",token))return 0x341;
  if(tokenIdentCompCI("mcause",token))return 0x342;
  if(tokenIdentCompCI("mtval",token))return 0x343;
  if(tokenIdentCompCI("mip",token))return 0x344;
  if(tokenIdentCompCI("mtinst",token))return 0x34A;
  if(tokenIdentCompCI("mtval2",token))return 0x34B;
  // Machine Configuration
  if(tokenIdentCompCI("menvcfg",token))return 0x30A;
  if(tokenIdentCompCI("menvcfgh",token))return 0x31A;
  if(tokenIdentCompCI("mseccfg",token))return 0x747;
  if(tokenIdentCompCI("mseccfgh",token))return 0x757;
  // Machine Memory Protection
  if(tokenIdentCompCI("pmpcfg0",token))return 0x3A0;
  if(tokenIdentCompCI("pmpcfg1",token))return 0x3A1;
  if(tokenIdentCompCI("pmpcfg2",token))return 0x3A2;
  if(tokenIdentCompCI("pmpcfg3",token))return 0x3A3;
  if(tokenIdentCompCI("pmpcfg4",token))return 0x3A4;
  if(tokenIdentCompCI("pmpcfg5",token))return 0x3A5;
  if(tokenIdentCompCI("pmpcfg6",token))return 0x3A6;
  if(tokenIdentCompCI("pmpcfg7",token))return 0x3A7;
  if(tokenIdentCompCI("pmpcfg8",token))return 0x3A8;
  if(tokenIdentCompCI("pmpcfg9",token))return 0x3A9;
  if(tokenIdentCompCI("pmpcfgA",token))return 0x3AA;
  if(tokenIdentCompCI("pmpcfgB",token))return 0x3AB;
  if(tokenIdentCompCI("pmpcfgC",token))return 0x3AC;
  if(tokenIdentCompCI("pmpcfgD",token))return 0x3AD;
  if(tokenIdentCompCI("pmpcfgE",token))return 0x3AE;
  if(tokenIdentCompCI("pmpcfgF",token))return 0x3AF;

  if(tokenIdentCompCI("pmpaddr0",token))return 0x3B0;
  if(tokenIdentCompCI("pmpaddr1",token))return 0x3B1;
  if(tokenIdentCompCI("pmpaddr2",token))return 0x3B2;
  if(tokenIdentCompCI("pmpaddr3",token))return 0x3B3;
  if(tokenIdentCompCI("pmpaddr4",token))return 0x3B4;
  if(tokenIdentCompCI("pmpaddr5",token))return 0x3B5;
  if(tokenIdentCompCI("pmpaddr6",token))return 0x3B6;
  if(tokenIdentCompCI("pmpaddr7",token))return 0x3B7;
  if(tokenIdentCompCI("pmpaddr8",token))return 0x3B8;
  if(tokenIdentCompCI("pmpaddr9",token))return 0x3B9;
  if(tokenIdentCompCI("pmpaddr10",token))return 0x3BA;
  if(tokenIdentCompCI("pmpaddr11",token))return 0x3BB;
  if(tokenIdentCompCI("pmpaddr12",token))return 0x3BC;
  if(tokenIdentCompCI("pmpaddr13",token))return 0x3BD;
  if(tokenIdentCompCI("pmpaddr14",token))return 0x3BE;
  if(tokenIdentCompCI("pmpaddr15",token))return 0x3BF;
  if(tokenIdentCompCI("pmpaddr16",token))return 0x3C0;
  if(tokenIdentCompCI("pmpaddr17",token))return 0x3C1;
  if(tokenIdentCompCI("pmpaddr18",token))return 0x3C2;
  if(tokenIdentCompCI("pmpaddr19",token))return 0x3C3;
  if(tokenIdentCompCI("pmpaddr20",token))return 0x3C4;
  if(tokenIdentCompCI("pmpaddr21",token))return 0x3C5;
  if(tokenIdentCompCI("pmpaddr22",token))return 0x3C6;
  if(tokenIdentCompCI("pmpaddr23",token))return 0x3C7;
  if(tokenIdentCompCI("pmpaddr24",token))return 0x3C8;
  if(tokenIdentCompCI("pmpaddr25",token))return 0x3C9;
  if(tokenIdentCompCI("pmpaddr26",token))return 0x3CA;
  if(tokenIdentCompCI("pmpaddr27",token))return 0x3CB;
  if(tokenIdentCompCI("pmpaddr28",token))return 0x3CC;
  if(tokenIdentCompCI("pmpaddr29",token))return 0x3CD;
  if(tokenIdentCompCI("pmpaddr30",token))return 0x3CE;
  if(tokenIdentCompCI("pmpaddr31",token))return 0x3CF;
  if(tokenIdentCompCI("pmpaddr32",token))return 0x3D0;
  if(tokenIdentCompCI("pmpaddr33",token))return 0x3D1;
  if(tokenIdentCompCI("pmpaddr34",token))return 0x3D2;
  if(tokenIdentCompCI("pmpaddr35",token))return 0x3D3;
  if(tokenIdentCompCI("pmpaddr36",token))return 0x3D4;
  if(tokenIdentCompCI("pmpaddr37",token))return 0x3D5;
  if(tokenIdentCompCI("pmpaddr38",token))return 0x3D6;
  if(tokenIdentCompCI("pmpaddr39",token))return 0x3D7;
  if(tokenIdentCompCI("pmpaddr40",token))return 0x3D8;
  if(tokenIdentCompCI("pmpaddr41",token))return 0x3D9;
  if(tokenIdentCompCI("pmpaddr42",token))return 0x3DA;
  if(tokenIdentCompCI("pmpaddr43",token))return 0x3DB;
  if(tokenIdentCompCI("pmpaddr44",token))return 0x3DC;
  if(tokenIdentCompCI("pmpaddr45",token))return 0x3DD;
  if(tokenIdentCompCI("pmpaddr46",token))return 0x3DE;
  if(tokenIdentCompCI("pmpaddr47",token))return 0x3DF;
  if(tokenIdentCompCI("pmpaddr48",token))return 0x3E0;
  if(tokenIdentCompCI("pmpaddr49",token))return 0x3E1;
  if(tokenIdentCompCI("pmpaddr50",token))return 0x3E2;
  if(tokenIdentCompCI("pmpaddr51",token))return 0x3E3;
  if(tokenIdentCompCI("pmpaddr52",token))return 0x3E4;
  if(tokenIdentCompCI("pmpaddr53",token))return 0x3E5;
  if(tokenIdentCompCI("pmpaddr54",token))return 0x3E6;
  if(tokenIdentCompCI("pmpaddr55",token))return 0x3E7;
  if(tokenIdentCompCI("pmpaddr56",token))return 0x3E8;
  if(tokenIdentCompCI("pmpaddr57",token))return 0x3E9;
  if(tokenIdentCompCI("pmpaddr58",token))return 0x3EA;
  if(tokenIdentCompCI("pmpaddr59",token))return 0x3EB;
  if(tokenIdentCompCI("pmpaddr60",token))return 0x3EC;
  if(tokenIdentCompCI("pmpaddr61",token))return 0x3ED;
  if(tokenIdentCompCI("pmpaddr62",token))return 0x3EE;
  if(tokenIdentCompCI("pmpaddr63",token))return 0x3EF;
  // Machine State Enable Registers
  if(tokenIdentCompCI("mstateen0",token))return 0x30C;
  if(tokenIdentCompCI("mstateen1",token))return 0x30D;
  if(tokenIdentCompCI("mstateen2",token))return 0x30E;
  if(tokenIdentCompCI("mstateen3",token))return 0x30F;
  if(tokenIdentCompCI("mstateen0h",token))return 0x31C;
  if(tokenIdentCompCI("mstateen1h",token))return 0x31D;
  if(tokenIdentCompCI("mstateen2h",token))return 0x31E;
  if(tokenIdentCompCI("mstateen3h",token))return 0x31F;
  // Machine Non-Maskable Interrupt Handling
  if(tokenIdentCompCI("mnscratch",token))return 0x740;
  if(tokenIdentCompCI("mnepc",token))return 0x741;
  if(tokenIdentCompCI("mncause",token))return 0x742;
  if(tokenIdentCompCI("mnstatus",token))return 0x744;
  // Machine Counter/Timers
  if(tokenIdentCompCI("mcycle",token))return 0xB00;
  if(tokenIdentCompCI("minstret",token))return 0xB02;
  if(tokenIdentCompCI("mhpmcounter3",token))return 0xB03;
  if(tokenIdentCompCI("mhpmcounter4",token))return 0xB04;
  if(tokenIdentCompCI("mhpmcounter5",token))return 0xB05;
  if(tokenIdentCompCI("mhpmcounter6",token))return 0xB06;
  if(tokenIdentCompCI("mhpmcounter7",token))return 0xB07;
  if(tokenIdentCompCI("mhpmcounter8",token))return 0xB08;
  if(tokenIdentCompCI("mhpmcounter9",token))return 0xB09;
  if(tokenIdentCompCI("mhpmcounter10",token))return 0xB0A;
  if(tokenIdentCompCI("mhpmcounter11",token))return 0xB0B;
  if(tokenIdentCompCI("mhpmcounter12",token))return 0xB0C;
  if(tokenIdentCompCI("mhpmcounter13",token))return 0xB0D;
  if(tokenIdentCompCI("mhpmcounter14",token))return 0xB0E;
  if(tokenIdentCompCI("mhpmcounter15",token))return 0xB0F;
  if(tokenIdentCompCI("mhpmcounter16",token))return 0xB10;
  if(tokenIdentCompCI("mhpmcounter17",token))return 0xB11;
  if(tokenIdentCompCI("mhpmcounter18",token))return 0xB12;
  if(tokenIdentCompCI("mhpmcounter19",token))return 0xB13;
  if(tokenIdentCompCI("mhpmcounter20",token))return 0xB14;
  if(tokenIdentCompCI("mhpmcounter21",token))return 0xB15;
  if(tokenIdentCompCI("mhpmcounter22",token))return 0xB16;
  if(tokenIdentCompCI("mhpmcounter23",token))return 0xB17;
  if(tokenIdentCompCI("mhpmcounter24",token))return 0xB18;
  if(tokenIdentCompCI("mhpmcounter25",token))return 0xB19;
  if(tokenIdentCompCI("mhpmcounter26",token))return 0xB1A;
  if(tokenIdentCompCI("mhpmcounter27",token))return 0xB1B;
  if(tokenIdentCompCI("mhpmcounter28",token))return 0xB1C;
  if(tokenIdentCompCI("mhpmcounter29",token))return 0xB1D;
  if(tokenIdentCompCI("mhpmcounter30",token))return 0xB1E;
  if(tokenIdentCompCI("mhpmcounter31",token))return 0xB1F;
  if(tokenIdentCompCI("mcycleh",token))return 0xB80;
  if(tokenIdentCompCI("minstreth",token))return 0xB82;
  if(tokenIdentCompCI("mhpmcounter3h",token))return 0xB83;
  if(tokenIdentCompCI("mhpmcounter4h",token))return 0xB84;
  if(tokenIdentCompCI("mhpmcounter5h",token))return 0xB85;
  if(tokenIdentCompCI("mhpmcounter6h",token))return 0xB86;
  if(tokenIdentCompCI("mhpmcounter7h",token))return 0xB87;
  if(tokenIdentCompCI("mhpmcounter8h",token))return 0xB88;
  if(tokenIdentCompCI("mhpmcounter9h",token))return 0xB89;
  if(tokenIdentCompCI("mhpmcounter10h",token))return 0xB8A;
  if(tokenIdentCompCI("mhpmcounter11h",token))return 0xB8B;
  if(tokenIdentCompCI("mhpmcounter12h",token))return 0xB8C;
  if(tokenIdentCompCI("mhpmcounter13h",token))return 0xB8D;
  if(tokenIdentCompCI("mhpmcounter14h",token))return 0xB8E;
  if(tokenIdentCompCI("mhpmcounter15h",token))return 0xB8F;
  if(tokenIdentCompCI("mhpmcounter16h",token))return 0xB90;
  if(tokenIdentCompCI("mhpmcounter17h",token))return 0xB91;
  if(tokenIdentCompCI("mhpmcounter18h",token))return 0xB92;
  if(tokenIdentCompCI("mhpmcounter19h",token))return 0xB93;
  if(tokenIdentCompCI("mhpmcounter20h",token))return 0xB94;
  if(tokenIdentCompCI("mhpmcounter21h",token))return 0xB95;
  if(tokenIdentCompCI("mhpmcounter22h",token))return 0xB96;
  if(tokenIdentCompCI("mhpmcounter23h",token))return 0xB97;
  if(tokenIdentCompCI("mhpmcounter24h",token))return 0xB98;
  if(tokenIdentCompCI("mhpmcounter25h",token))return 0xB99;
  if(tokenIdentCompCI("mhpmcounter26h",token))return 0xB9A;
  if(tokenIdentCompCI("mhpmcounter27h",token))return 0xB9B;
  if(tokenIdentCompCI("mhpmcounter28h",token))return 0xB9C;
  if(tokenIdentCompCI("mhpmcounter29h",token))return 0xB9D;
  if(tokenIdentCompCI("mhpmcounter30h",token))return 0xB9E;
  if(tokenIdentCompCI("mhpmcounter31h",token))return 0xB9F;
  // Machine Counter Setup
  if(tokenIdentCompCI("mcountinhibit",token))return 0x320;
  if(tokenIdentCompCI("mhpmevent3",token))return 0x323;
  if(tokenIdentCompCI("mhpmevent4",token))return 0x324;
  if(tokenIdentCompCI("mhpmevent5",token))return 0x325;
  if(tokenIdentCompCI("mhpmevent6",token))return 0x326;
  if(tokenIdentCompCI("mhpmevent7",token))return 0x327;
  if(tokenIdentCompCI("mhpmevent8",token))return 0x328;
  if(tokenIdentCompCI("mhpmevent9",token))return 0x329;
  if(tokenIdentCompCI("mhpmevent10",token))return 0x32A;
  if(tokenIdentCompCI("mhpmevent11",token))return 0x32B;
  if(tokenIdentCompCI("mhpmevent12",token))return 0x32C;
  if(tokenIdentCompCI("mhpmevent13",token))return 0x32D;
  if(tokenIdentCompCI("mhpmevent14",token))return 0x32E;
  if(tokenIdentCompCI("mhpmevent15",token))return 0x32F;
  if(tokenIdentCompCI("mhpmevent16",token))return 0x330;
  if(tokenIdentCompCI("mhpmevent17",token))return 0x331;
  if(tokenIdentCompCI("mhpmevent18",token))return 0x332;
  if(tokenIdentCompCI("mhpmevent19",token))return 0x333;
  if(tokenIdentCompCI("mhpmevent20",token))return 0x334;
  if(tokenIdentCompCI("mhpmevent21",token))return 0x335;
  if(tokenIdentCompCI("mhpmevent22",token))return 0x336;
  if(tokenIdentCompCI("mhpmevent23",token))return 0x337;
  if(tokenIdentCompCI("mhpmevent24",token))return 0x338;
  if(tokenIdentCompCI("mhpmevent25",token))return 0x339;
  if(tokenIdentCompCI("mhpmevent26",token))return 0x33A;
  if(tokenIdentCompCI("mhpmevent27",token))return 0x33B;
  if(tokenIdentCompCI("mhpmevent28",token))return 0x33C;
  if(tokenIdentCompCI("mhpmevent29",token))return 0x33D;
  if(tokenIdentCompCI("mhpmevent30",token))return 0x33E;
  if(tokenIdentCompCI("mhpmevent31",token))return 0x33F;
  if(tokenIdentCompCI("mhpmevent3h",token))return 0x723;
  if(tokenIdentCompCI("mhpmevent4h",token))return 0x724;
  if(tokenIdentCompCI("mhpmevent5h",token))return 0x725;
  if(tokenIdentCompCI("mhpmevent6h",token))return 0x726;
  if(tokenIdentCompCI("mhpmevent7h",token))return 0x727;
  if(tokenIdentCompCI("mhpmevent8h",token))return 0x728;
  if(tokenIdentCompCI("mhpmevent9h",token))return 0x729;
  if(tokenIdentCompCI("mhpmevent10h",token))return 0x72A;
  if(tokenIdentCompCI("mhpmevent11h",token))return 0x72B;
  if(tokenIdentCompCI("mhpmevent12h",token))return 0x72C;
  if(tokenIdentCompCI("mhpmevent13h",token))return 0x72D;
  if(tokenIdentCompCI("mhpmevent14h",token))return 0x72E;
  if(tokenIdentCompCI("mhpmevent15h",token))return 0x72F;
  if(tokenIdentCompCI("mhpmevent16h",token))return 0x730;
  if(tokenIdentCompCI("mhpmevent17h",token))return 0x731;
  if(tokenIdentCompCI("mhpmevent18h",token))return 0x732;
  if(tokenIdentCompCI("mhpmevent19h",token))return 0x733;
  if(tokenIdentCompCI("mhpmevent20h",token))return 0x734;
  if(tokenIdentCompCI("mhpmevent21h",token))return 0x735;
  if(tokenIdentCompCI("mhpmevent22h",token))return 0x736;
  if(tokenIdentCompCI("mhpmevent23h",token))return 0x737;
  if(tokenIdentCompCI("mhpmevent24h",token))return 0x738;
  if(tokenIdentCompCI("mhpmevent25h",token))return 0x739;
  if(tokenIdentCompCI("mhpmevent26h",token))return 0x73A;
  if(tokenIdentCompCI("mhpmevent27h",token))return 0x73B;
  if(tokenIdentCompCI("mhpmevent28h",token))return 0x73C;
  if(tokenIdentCompCI("mhpmevent29h",token))return 0x73D;
  if(tokenIdentCompCI("mhpmevent30h",token))return 0x73E;
  if(tokenIdentCompCI("mhpmevent31h",token))return 0x73F;
  // Debug/Trace Registers (shared with Debug Mode)
  if(tokenIdentCompCI("tdata0",token))return 0x7A0;
  if(tokenIdentCompCI("tdata1",token))return 0x7A1;
  if(tokenIdentCompCI("tdata2",token))return 0x7A2;
  if(tokenIdentCompCI("tdata3",token))return 0x7A3;
  if(tokenIdentCompCI("mcontext",token))return 0x7A8;
  // Debug Mode Registers
  if(tokenIdentCompCI("dcsr",token))return 0x7B0;
  if(tokenIdentCompCI("dpc",token))return 0x7B1;
  if(tokenIdentCompCI("dscratch0",token))return 0x7B2;
  if(tokenIdentCompCI("dscratch1",token))return 0x7B3;

  compError("Unknown CSR Register",token);
  return 0;
}

void encodeCsrRegister(CompContext*ctx,uint32_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseCsr(ctx->token) << 20;
  nextTokenEnforceComma(ctx);
  enc += parseIntReg(ctx->token) << 15;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeCsrImmediate(CompContext*ctx,uint32_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 7;
  nextTokenEnforceComma(ctx);
  enc += parseCsr(ctx->token) << 20;
  nextTokenEnforceComma(ctx);
  enc += parseUImm(ctx->token,5) << 15;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32Zicsr(CompContext*ctx){
  if     (tokenIdentCompCI("csrrw", ctx->token))encodeCsrRegister(ctx,0x00001073);
  else if(tokenIdentCompCI("csrrs", ctx->token))encodeCsrRegister(ctx,0x00002073);
  else if(tokenIdentCompCI("csrrc", ctx->token))encodeCsrRegister(ctx,0x00003073);
  else if(tokenIdentCompCI("csrrwi",ctx->token))encodeCsrImmediate(ctx,0x00005073);
  else if(tokenIdentCompCI("csrrsi",ctx->token))encodeCsrImmediate(ctx,0x00006073);
  else if(tokenIdentCompCI("csrrci",ctx->token))encodeCsrImmediate(ctx,0x00007073);
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
