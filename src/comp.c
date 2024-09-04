
#include"common_types.h"
#include"comp.h"
#include"token.h"
#include"util.h"
#include"export.h"
#include"data.h"
#include"bss.h"
#include"riscv.h"
#include"section.h"
#include"symbol.h"
#include"constants.h"

#include<stdlib.h>

void compPass(CompContext*ctx){
  ctx->token = ctx->tokenHead;
  ctx->section = NULL;

  while(ctx->token){
   
    if(ctx->token->type == Newline){
      ctx->token = ctx->token->next;
      continue;
    }

    // Sections

    if(tokenIdentComp(".text",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".text",SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,0,0,4096,TEXT);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".data",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".data",SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,DATA);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".rodata",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".rodata",SHT_PROGBITS,SHF_ALLOC,0,0,0,4096,DATA);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".bss",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".bss",SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,BSS);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".section",ctx->token)){
      compError("directive .section not implemented yet",ctx->token);
      //TODO Implement
    }

    // Common Directives
    if(tokenIdentComp(".equ",ctx->token)){
      nextTokenEnforceExistence(ctx);
      Token*nameToken = ctx->token;
      nextTokenEnforceComma(ctx);
      if(ctx->pass == INDEX)
	addConstant(ctx,nameToken,ctx->token);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    // Extern Symbols
    if(tokenIdentComp(".extern",ctx->token)){
      nextTokenEnforceExistence(ctx);
      if(ctx->token->type != Identifier)
	compError("Identifier expected after .extern directive",ctx->token);
      addSymbol(ctx,ctx->token,0,0,STT_NOTYPE,STB_GLOBAL,STV_DEFAULT,0);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    // Guard against Assembling while no section is selected
    if(!ctx->section)
      compError("No Section selected to assemble into",ctx->token);

    // Symbols
    if(ctx->token->type == Identifier && ctx->token->next && ctx->token->next->type == Doubledot){
      addSymbol(
	  ctx,
	  ctx->token,
	  ctx->section->size,
	  0,
	  ctx->section->mode==TEXT ? STT_FUNC : STT_OBJECT,
	  STB_GLOBAL,
	  STV_DEFAULT,
	  ctx->section->sectionIndex
      );
      ctx->token = ctx->token->next;
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".align",ctx->token)){
      nextTokenEnforceExistence(ctx);

      Token*valTok = getNumberOrConstant(ctx);
      if(!valTok)
	compError("Number or Constant expected",ctx->token);

      if(ctx->pass==INDEX){
	ctx->section->size = align(ctx->section->size, parseUImm(valTok, 5));
      }
      else if(ctx->section->shdr.sh_type == SHT_NOBITS){
	ctx->section->index = align(ctx->section->index, parseUImm(valTok, 5));
      }
      else{
	uint32_t align_mask = (1 << parseUImm(valTok, 5)) -1;
	while(ctx->section->index & align_mask){
	  ctx->section->buff[ctx->section->index] = 0;
	  ctx->section->index ++;
	}
      }
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    // Data
    if(ctx->section->mode == DATA)
      if(compData(ctx))
	continue;
    // Bss
    if(ctx->section->mode == BSS)
      if(compBSS(ctx))
	continue;
    // Text
    if(ctx->section->mode == TEXT)
      if(compRV(ctx))
	continue;

    compError("Unexpected Token in Main Switch",ctx->token);
  }
}


void comp(char*inputfilename,char*outputfilename){
  // Create CompContext
  CompContext*ctx = malloc(sizeof(CompContext));
  ctx->constantHead = NULL;

  // Tokenize File
  ctx->tokenHead = tokenizeFile(inputfilename);

  // Create Unique Sections
  ctx->shnum = 0;
  ctx->sectionHead = 0;
  ctx->sectionTail = 0;
  ctx->size_shstrtab = 0;
  addSection(ctx,"",0,0,0,0,0,0,0);
  ctx->shstrtab = addSection(ctx,".shstrtab",SHT_STRTAB,0,0,0,0,0,0);
  ctx->strtab = addSection(ctx,".strtab",SHT_STRTAB,0,0,0,0,0,0);
  ctx->symtab = addSection(ctx,".symtab",SHT_SYMTAB,0,ctx->strtab->sectionIndex,0,sizeof(Elf32_Sym),0,0);

  // Create first, empty Symbol
  initSymbolList(ctx);

  // Pass Index: Create Sections, Symbols, Constants and estimate Buffer Sizes
  ctx->pass = INDEX;
  compPass(ctx);

  //Set Strtab size and symtab size and symbol count
  symbolPassPostIndex(ctx);

  // Shstrtab Size
  ctx->shstrtab->size += ctx->size_shstrtab;

  // Allocate section Buffers 
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next){
    if(sec->size != 0 && sec->shdr.sh_type != SHT_NOBITS){
      sec->buff = malloc(sec->size);
    }
  }

  // Shstrtab Content
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next){
    sec->shdr.sh_name = ctx->shstrtab->index;
    for(char*cp = sec->name;*cp!='\0';cp++){
      ctx->shstrtab->buff[ctx->shstrtab->index] = *cp;
      ctx->shstrtab->index++;
    }
    ctx->shstrtab->buff[ctx->shstrtab->index] = '\0';
    ctx->shstrtab->index++;
  }

  // Pass Comp: Insert Data into Buffers
  ctx->pass = COMP;
  compPass(ctx);

  // Insert Symbols into Symtab Section, Names into Strtab Section
  symbolPassPostComp(ctx);

  // Set Section Size
  for(Section*sec = ctx->sectionHead->next;sec;sec=sec->next)
    sec->shdr.sh_size = sec->index;

  // Set Section Offset
  ctx->sectionHead->next->shdr.sh_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ctx->shnum;
  for(Section*sec = ctx->sectionHead->next;sec->next;sec=sec->next){
    sec->next->shdr.sh_offset = sec->shdr.sh_offset + (sec->buff ? sec->index : 0);
  }

  // Export
  export_elf(ctx,outputfilename);

}

