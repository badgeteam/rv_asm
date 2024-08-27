
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

bool tryCompRelocation(CompContext*ctx,uint32_t type){
  struct Token*backupToken = ctx->token;
  struct Token*nameToken = NULL;
  int32_t addend = 0;
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
    case R_RISCV_LO12_I:
      if(!tokenIdentComp("lo",ctx->token))
	goto fail;
      break;
    case R_RISCV_LO12_S:
      if(!tokenIdentComp("lo",ctx->token))
	goto fail;
      break;
    case R_RISCV_PCREL_LO12_I:
      if(!tokenIdentComp("pcrel_lo",ctx->token))
	goto fail;
      break;
    case R_RISCV_PCREL_LO12_S:
      if(!tokenIdentComp("pcrel_lo",ctx->token))
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
  nameToken = ctx->token;
  if(!ctx->token->next)
    goto fail;
  ctx->token = ctx->token->next;

  if(ctx->token->type == Colon){
    // Modify Addend
    if(!ctx->token->next)
      goto fail;
    ctx->token = ctx->token->next;
    
    if(ctx->token->type != Number)
      goto fail;
    addend = parseInt(ctx->token);
    if(!ctx->token->next)
      goto fail;
    ctx->token = ctx->token->next;
  }

  if(ctx->token->type != BracketOut)
    goto fail;
  ctx->token = ctx->token->next;

  // Apply Relocation
  addRelaEntry(ctx,ctx->section->index,getSymbolIndex(ctx,nameToken),type,addend);

  return true;
fail:
  ctx->token = backupToken;
  return false;
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
  }
  else if(!tryCompRelocation(ctx,R_RISCV_HI20))
    compError("Number or \%hi relocation expected",ctx->token);

  insert4ByteCheckLineEnd(ctx,enc);
  return;
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
  }
  else if(!tryCompRelocation(ctx,R_RISCV_PCREL_HI20))
    compError("Number or \%pcrel_hi relocation expected",ctx->token);

  insert4ByteCheckLineEnd(ctx,enc);
  return;
}

void encodeU(CompContext*ctx, uint32_t enc){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  if(ctx->token->type == Number){
    compError("U encoding with immediate number not implemented. use symbols",ctx->token);
  }
  if(ctx->token->type == Identifier){
    addRelaEntry(ctx,ctx->section->index,getSymbolIndex(ctx,ctx->token),R_RISCV_JAL,0); 
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Symbol Name expected for relocation",ctx->token);
}

void encodeB(CompContext*ctx,uint32_t enc){
  if(!ctx->token->next)
    compError("UnexpectedEOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 15;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 20;
  ctx->token = nextTokenEnforceColon(ctx->token);
  if(ctx->token->type == Number)
    compError("B Encoding with immediate number not implemented. use symbols",ctx->token);
  if(ctx->token->type == Identifier){
    addRelaEntry(ctx,ctx->section->index,getSymbolIndex(ctx,ctx->token),R_RISCV_BRANCH,0);
    ctx->token = ctx->token->next;
    insert4ByteCheckLineEnd(ctx,enc);
    return;
  }
  compError("Symbol Name expected for relocation",ctx->token);
}

void encodeS(CompContext*ctx,uint32_t enc){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 15;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 20;
  if(ctx->token->next && ctx->token->next->type != Newline){
    ctx->token = nextTokenEnforceColon(ctx->token);
    if(ctx->token->type == Number){
      uint32_t imm = parseImm(ctx->token,12);
      enc += (imm & 0x1F) << 7;
      enc += (imm >> 5) << 25;
      ctx->token = ctx->token->next;
    }
    else if(tryCompRelocation(ctx,R_RISCV_LO12_S));
    else if(tryCompRelocation(ctx,R_RISCV_PCREL_LO12_S));
    else
     compError("The offset of an S encoding has to be a number, a \%lo(), a \%pcrel_lo() or nothing",ctx->token);
  }else{
    ctx->token = ctx->token->next;
  }
  insert4ByteCheckLineEnd(ctx,enc);
}


void encodeStore(CompContext*ctx,uint32_t enc){
  nextTokenEnforceExistence(ctx);
  enc += parseIntReg(ctx->token) << 20;	// rs2 
  ctx->token = nextTokenEnforceColon(ctx->token);

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

void encodeI(CompContext*ctx,uint32_t enc){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 15;
  if(ctx->token->next && ctx->token->next->type != Newline){
    ctx->token = nextTokenEnforceColon(ctx->token);
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
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseIntReg(ctx->token) << 15;
  ctx->token = nextTokenEnforceColon(ctx->token);
  enc += parseUImm(ctx->token,shamt) << 20;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

void encodeFence(CompContext*ctx){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  uint32_t enc = 0x0F;
  enc += parseIORW(ctx->token) << 20;
  ctx->token = nextTokenEnforceColon(ctx->token);
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
  else if(tokenIdentCompCI("jalr" ,ctx->token))encodeI(ctx,0x67);
  else if(tokenIdentCompCI("beq"  ,ctx->token))encodeB(ctx,0x0063);
  else if(tokenIdentCompCI("bne"  ,ctx->token))encodeB(ctx,0x1063);
  else if(tokenIdentCompCI("blt"  ,ctx->token))encodeB(ctx,0x4063);
  else if(tokenIdentCompCI("bge"  ,ctx->token))encodeB(ctx,0x5063);
  else if(tokenIdentCompCI("bltu" ,ctx->token))encodeB(ctx,0x6063);
  else if(tokenIdentCompCI("bleu" ,ctx->token))encodeB(ctx,0x7063);
  else if(tokenIdentCompCI("lb"   ,ctx->token))encodeI(ctx,0x0003);
  else if(tokenIdentCompCI("lh"   ,ctx->token))encodeI(ctx,0x1003);
  else if(tokenIdentCompCI("lw"   ,ctx->token))encodeI(ctx,0x2003);
  else if(tokenIdentCompCI("lbu"  ,ctx->token))encodeI(ctx,0x4003);
  else if(tokenIdentCompCI("lhu"  ,ctx->token))encodeI(ctx,0x5003);
  else if(tokenIdentCompCI("sb"   ,ctx->token))encodeStore(ctx,0x0023);
  else if(tokenIdentCompCI("sh"   ,ctx->token))encodeStore(ctx,0x1023);
  else if(tokenIdentCompCI("sw"   ,ctx->token))encodeStore(ctx,0x2023);
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

void encodeAtomic(CompContext*ctx,uint32_t enc,bool load){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
  enc += parseIntReg(ctx->token) << 7;
  ctx->token = nextTokenEnforceColon(ctx->token);
  if(!load){
    enc += parseIntReg(ctx->token) << 20;
    ctx->token = nextTokenEnforceColon(ctx->token);
  }
  enc += parseBracketReg(ctx) << 15;
  ctx->token = ctx->token->next;
  insert4ByteCheckLineEnd(ctx,enc);
}

bool compRV32A(CompContext*ctx){
  if     (tokenIdentCompCI("lr.w",	     ctx->token))encodeAtomic(ctx,0x1000202F,true);
  else if(tokenIdentCompCI("lr.w.aq",	     ctx->token))encodeAtomic(ctx,0x1200202F,true);
  else if(tokenIdentCompCI("lr.w.rl",	     ctx->token))encodeAtomic(ctx,0x1400202F,true);
  else if(tokenIdentCompCI("lr.w.aq.rl",     ctx->token))encodeAtomic(ctx,0x1600202F,true);
  else if(tokenIdentCompCI("sc.w",           ctx->token))encodeAtomic(ctx,0x1800202F,false);
  else if(tokenIdentCompCI("sc.w.aq",        ctx->token))encodeAtomic(ctx,0x1A00202F,false);
  else if(tokenIdentCompCI("sc.w.rl",        ctx->token))encodeAtomic(ctx,0x1C00202F,false);
  else if(tokenIdentCompCI("sc.w.aq.rl",     ctx->token))encodeAtomic(ctx,0x1E00202F,false);
  else if(tokenIdentCompCI("amoswap.w",	     ctx->token))encodeAtomic(ctx,0x0800202F,false);
  else if(tokenIdentCompCI("amoswap.w.aq",   ctx->token))encodeAtomic(ctx,0x0A00202F,false);
  else if(tokenIdentCompCI("amoswap.w.rl",   ctx->token))encodeAtomic(ctx,0x0C00202F,false);
  else if(tokenIdentCompCI("amoswap.w.aq.rl",ctx->token))encodeAtomic(ctx,0x0E00202F,false);
  else if(tokenIdentCompCI("amoadd.w",       ctx->token))encodeAtomic(ctx,0x0000202F,false);
  else if(tokenIdentCompCI("amoadd.w.aq",    ctx->token))encodeAtomic(ctx,0x0200202F,false);
  else if(tokenIdentCompCI("amoadd.w.rl",    ctx->token))encodeAtomic(ctx,0x0400202F,false);
  else if(tokenIdentCompCI("amoadd.w.aq.rl", ctx->token))encodeAtomic(ctx,0x0600202F,false);
  else if(tokenIdentCompCI("amoxor.w",       ctx->token))encodeAtomic(ctx,0x2000202F,false);
  else if(tokenIdentCompCI("amoxor.w.aq",    ctx->token))encodeAtomic(ctx,0x2200202F,false);
  else if(tokenIdentCompCI("amoxor.w.rl",    ctx->token))encodeAtomic(ctx,0x2400202F,false);
  else if(tokenIdentCompCI("amoxor.w.aq.rl", ctx->token))encodeAtomic(ctx,0x2600202F,false);
  else if(tokenIdentCompCI("amoand.w",       ctx->token))encodeAtomic(ctx,0x6000202F,false);
  else if(tokenIdentCompCI("amoand.w.aq",    ctx->token))encodeAtomic(ctx,0x6200202F,false);
  else if(tokenIdentCompCI("amoand.w.rl",    ctx->token))encodeAtomic(ctx,0x6400202F,false);
  else if(tokenIdentCompCI("amoand.w.aq.rl", ctx->token))encodeAtomic(ctx,0x6600202F,false);
  else if(tokenIdentCompCI("amoor.w",        ctx->token))encodeAtomic(ctx,0x4000202F,false);
  else if(tokenIdentCompCI("amoor.w.aq",     ctx->token))encodeAtomic(ctx,0x4200202F,false);
  else if(tokenIdentCompCI("amoor.w.rl",     ctx->token))encodeAtomic(ctx,0x4400202F,false);
  else if(tokenIdentCompCI("amoor.w.aq.rl",  ctx->token))encodeAtomic(ctx,0x4600202F,false);
  else if(tokenIdentCompCI("amomin.w",       ctx->token))encodeAtomic(ctx,0x8000202F,false);
  else if(tokenIdentCompCI("amomin.w.aq",    ctx->token))encodeAtomic(ctx,0x8200202F,false);
  else if(tokenIdentCompCI("amomin.w.rl",    ctx->token))encodeAtomic(ctx,0x8400202F,false);
  else if(tokenIdentCompCI("amomin.w.aq.rl", ctx->token))encodeAtomic(ctx,0x8600202F,false);
  else if(tokenIdentCompCI("amomax.w",       ctx->token))encodeAtomic(ctx,0xA000202F,false);
  else if(tokenIdentCompCI("amomax.w.aq",    ctx->token))encodeAtomic(ctx,0xA200202F,false);
  else if(tokenIdentCompCI("amomax.w.rl",    ctx->token))encodeAtomic(ctx,0xA400202F,false);
  else if(tokenIdentCompCI("amomax.w.aq.rl", ctx->token))encodeAtomic(ctx,0xA600202F,false);
  else if(tokenIdentCompCI("amominu.w",      ctx->token))encodeAtomic(ctx,0xC000202F,false);
  else if(tokenIdentCompCI("amominu.w.aq",   ctx->token))encodeAtomic(ctx,0xC200202F,false);
  else if(tokenIdentCompCI("amominu.w.rl",   ctx->token))encodeAtomic(ctx,0xC400202F,false);
  else if(tokenIdentCompCI("amominu.w.aq.rl",ctx->token))encodeAtomic(ctx,0xC600202F,false);
  else if(tokenIdentCompCI("amomaxu.w",      ctx->token))encodeAtomic(ctx,0xE000202F,false);
  else if(tokenIdentCompCI("amomaxu.w.aq",   ctx->token))encodeAtomic(ctx,0xE200202F,false);
  else if(tokenIdentCompCI("amomaxu.w.rl",   ctx->token))encodeAtomic(ctx,0xE400202F,false);
  else if(tokenIdentCompCI("amomaxu.w.aq.rl",ctx->token))encodeAtomic(ctx,0xE600202F,false);
  else return false;
  return true;
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
