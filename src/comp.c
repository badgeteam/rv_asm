
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
#include<stdio.h>

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
	ctx->section = addSection(ctx,".text",SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,0,0,4096);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".data",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".data",SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".rodata",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".rodata",SHT_PROGBITS,SHF_ALLOC,0,0,0,4096);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".bss",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".bss",SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096);
      nextTokenEnforceNewlineEOF(ctx);
      continue;
    }

    if(tokenIdentComp(".section",ctx->token)){
      nextTokenEnforceExistence(ctx);
      if(ctx->token->type != Identifier)
	compError("Identifier expected",ctx->token);
      Token*nameToken = ctx->token;

      // Select Section if it exists
      if((ctx->section = getSectionByIdentifier(ctx))){
	while(ctx->token->next && ctx->token->next->type != Newline)
	  ctx->token = ctx->token->next;
	ctx->token = ctx->token->next;
	continue;
      }

      // Create Section otherwise
      uint32_t section_flags = 0,section_type = 0;
      if(ctx->token->next && ctx->token->next->type == Comma){
	nextTokenEnforceComma(ctx);
	if(ctx->token->type != String)
	  compError("String expected",ctx->token);
	for(char*cp = ctx->token->buff + 1; cp < ctx->token->buffTop -1; cp++){
	  switch(*cp){
	    case'a': section_flags |= SHF_ALLOC; break;
	    case'w': section_flags |= SHF_WRITE; break;
	    case'x': section_flags |= SHF_EXECINSTR; break;
	    default: compError("section flags must be any combination of a, w, x",ctx->token);
	  }
	}
	if(ctx->token->next && ctx->token->next->type == Comma){
	  nextTokenEnforceComma(ctx);
	  if(tokenIdentComp("@progbits",ctx->token))
	    section_type = SHT_PROGBITS;
	  else if(tokenIdentComp("@nobits",ctx->token))
	    section_type = SHT_NOBITS;
	}
      }
      nextTokenEnforceNewlineEOF(ctx);
      if(section_flags == 0 || section_type == 0){
	if(tokenIdentCompPartial(".text",nameToken,0)){
	  section_flags = section_flags==0 ? SHF_ALLOC|SHF_EXECINSTR : section_flags;
	  section_type = section_type == 0 ? SHT_PROGBITS : section_type;
	}
	else if(tokenIdentCompPartial(".data",nameToken,0)){
	  section_flags = section_flags==0 ? SHF_ALLOC|SHF_WRITE : section_flags;
	  section_type = section_type == 0 ? SHT_PROGBITS : section_type;
	}
	else if(tokenIdentCompPartial(".rodata",nameToken,0)){
	  section_flags = section_flags==0 ? SHF_ALLOC : section_flags;
	  section_type = section_type == 0 ? SHT_PROGBITS : section_type;
	}
	else if(tokenIdentCompPartial(".bss",nameToken,0)){
	  section_flags = section_flags==0 ? SHF_ALLOC|SHF_WRITE : section_flags;
	  section_type = section_type == 0 ? SHT_NOBITS : section_type;
	}
	else
	 compError("Sections whoose name does not begin with .text, .data, .rodata or .bss must have flags and type specified",ctx->token);
      }
      ctx->section = addSection(ctx,copyTokenContent(nameToken),section_type,section_flags,0,0,0,4096);
      continue;
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

    if(tryCompSymbolDirectives(ctx))
      continue;


    if(!ctx->section)
      compError("No Section selected",ctx->token);

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

    
    // BSS
    if(ctx->section->shdr.sh_type == SHT_PROGBITS){
      if(ctx->section->shdr.sh_flags & SHF_EXECINSTR){
	if(compRV(ctx))
	  continue;
      }
      else{
	if(compData(ctx))
	  continue;
      }
    }else if(ctx->section->shdr.sh_type == SHT_NOBITS){
      if(compBSS(ctx))
	continue;
    }

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
  addSection(ctx,"",0,0,0,0,0,0);
  ctx->shstrtab = addSection(ctx,".shstrtab",SHT_STRTAB,0,0,0,0,0);
  ctx->strtab = addSection(ctx,".strtab",SHT_STRTAB,0,0,0,0,0);
  ctx->symtab = addSection(ctx,".symtab",SHT_SYMTAB,0,ctx->strtab->sectionIndex,0,sizeof(Elf32_Sym),0);

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

