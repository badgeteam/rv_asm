
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
#include"lr.h"


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
  
    if(tryCompSectionDirectives(ctx))
      continue;

    if(tryCompSymbolDirectives(ctx))
      continue;

    if(tryCompEquSet(ctx))
      continue;

    if(tokenIdentComp(".align",ctx->token)){
      if(!ctx->section)
	compError("No Section selected",ctx->token);

      nextTokenEnforceExistence(ctx);

      if(!lrParseNumber(ctx))
	compError("Arithmetic Expression expeted",ctx->token);
      enforceNewlineEOF(ctx);

      if(ctx->pass==INDEX){
	ctx->section->size = align(ctx->section->size, lrGetUImm(ctx, 5));
      }
      else if(ctx->section->shdr.sh_type == SHT_NOBITS){
	ctx->section->index = align(ctx->section->index, lrGetUImm(ctx, 5));
      }
      else{
	uint32_t align_mask = (1 << lrGetUImm(ctx, 5)) -1;
	while(ctx->section->index & align_mask){
	  ctx->section->buff[ctx->section->index] = 0;
	  ctx->section->index ++;
	}
      }
      continue;
    }

    if(ctx->section){

      if(ctx->section->shdr.sh_type == SHT_PROGBITS){
	if(ctx->section->shdr.sh_flags & SHF_EXECINSTR){
	  if(compRV(ctx))
	    continue;
	}
	else{
	  if(compData(ctx))
	    continue;
	}
      }
      else if(ctx->section->shdr.sh_type == SHT_NOBITS){
	if(compBSS(ctx))
	  continue;
      }

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

