#include"relocation.h"
#include "elf.h"
#include"token.h"
#include"section.h"
#include"symbol.h"
#include"lr.h"
#include<stdlib.h>


void addRelaEntry(CompContext*ctx,uint32_t offset, Symbol*sym, uint32_t type, int32_t addend){
  if(!ctx->section->rela){
    uint32_t name_size_index = 5;	// sizeof(".rela")
    for(char*cp = ctx->section->name; *cp!='\0'; cp++)
      name_size_index++;
    name_size_index++;
    char*name = malloc(name_size_index);
    name[0] = '.';
    name[1] = 'r';
    name[2] = 'e';
    name[3] = 'l';
    name[4] = 'a';
    name_size_index = 5;
    for(char*cp = ctx->section->name; *cp!='\0'; cp++){
      name[name_size_index] = *cp;
      name_size_index++;
    }
    name[name_size_index] = '\0';

    ctx->section->rela = addSection(
	ctx,
	name,
	SHT_RELA,
	0,
	ctx->symtab->sectionIndex,
	ctx->section->sectionIndex,
	sizeof(Elf32_Rela),
	0
	);

  }

  if(ctx->pass == INDEX){
    ctx->section->rela->size += sizeof(Elf32_Rela);
  }
  else{
    Elf32_Rela*rela = (Elf32_Rela*)(ctx->section->rela->buff + ctx->section->rela->index);
    rela->r_offset = offset;
    rela->r_info = ELF32_R_INFO(sym->index,type);
    rela->r_addend = addend;
    ctx->section->rela->index += sizeof(Elf32_Rela);
  }
}

bool isRelTypePcrel(uint32_t type){
  switch(type){
    case R_RISCV_PCREL_HI20:
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S:
    case R_RISCV_32_PCREL:
      return true;
    default:
      return false;
  }
}

bool tryCompImplicitRelocation(CompContext*ctx,uint32_t type){
  Token*backupToken = ctx->token;
  if(!lrParseRelocation(ctx)) return false;
  if(isRelTypePcrel(type) != lrIsPcrel(ctx)){
    ctx->token = backupToken;
    return false;
  }
  addRelaEntry(
      ctx,
      ctx->section->index,
      getSymbol(ctx,lrGetSymbol(ctx)),
      type,
      lrGetInt(ctx)
      );
  return true;
}

bool tryCompRelocation(CompContext*ctx,uint32_t type){
  struct Token*backupToken = ctx->token;
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
  else if(type == R_RISCV_32){
    if(!tokenStrComp("32",ctx->token))
      goto fail;
  }else if(type == R_RISCV_32_PCREL){
    if(!tokenIdentComp("pcrel_32",ctx->token))
      goto fail;
  }
  else if(type == R_RISCV_GOT_HI20){
    if(!tokenIdentComp("got_pcrel_hi",ctx->token))
      goto fail;
  }
  else goto fail;

  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != BracketIn)
    goto fail;

  if(!lrParseRelocation(ctx))
    goto fail;
  if(lrIsPcrel(ctx))
    compError("Dot Symbols cannot occur inside a %relocation() statement",ctx->token);

  if(ctx->token->type != BracketOut)
    goto fail;
  ctx->token = ctx->token->next;


  // Apply Relocation
  addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,lrGetSymbol(ctx)),type,lrGetInt(ctx));
  return true;
fail:
  ctx->token = backupToken;
  return false;
}



