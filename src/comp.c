
#include<string.h>
#include<elf.h>

#include"comp.h"
#include"util.h"
#include"export.h"


void setSection(CompContext*ctx,size_t type,size_t flags){
  if(ctx->pass == INDEX_SECTIONS){
    ctx->shstrtab->size += ctx->token->buffTop - ctx->token->buff + 1;
    return;
  }

  Section*sec = ctx->sectionHead;
  // Try to select existing section
  while(sec){
    if(tokenIdentComp((char*)(ctx->shstrtab->buff + sec->name_offset), ctx->token)){
      ctx->section = sec;
      return;
    }
    sec=sec->next;
  }
  // Create New Section
  if(ctx->pass == INDEX_BUFFERS){
    // Allocate Section
    sec = malloc(sizeof(Section));
    sec->name_offset = ctx->shstrtab->index;
    sec->index=0;
    sec->size=0;
    sec->type = type;
    sec->flags = flags;
// sec->addr
// sec->offset Set after buffer allocation in pass Comp
// sec->link
// sec->entsize
    sec->next = NULL;
    ctx->shnum++;
    sec->sectionIndex = ctx->shnum;
    ctx->sectionTail->next = sec;
    ctx->sectionTail = sec;
    ctx->section = sec;
    // Write Name to shstrtab
    memcpy(ctx->shstrtab->buff + ctx->shstrtab->index, ctx->token->buff, ctx->token->buffTop - ctx->token->buff);
    ctx->shstrtab->index += ctx->token->buffTop - ctx->token->buff;
    ctx->shstrtab->buff[ctx->shstrtab->index] = '\0';
    ctx->shstrtab->index++;
  }
}

void compPass(CompContext*ctx){

  bool mode_text=false,mode_data=false;
  while(ctx->token){
    
    if(ctx->token->type == Newline){
      ctx->token = ctx->token->next;
      continue;
    }

    if(ctx->token->type == Identifier){

      if(ctx->token->next && ctx->token->next->type == Doubledot){
	// Symbol
	if(ctx->pass == INDEX_BUFFERS){
	  // TODO Add Symbol
	}
	ctx->token = ctx->token->next->next;
	continue;
      }

      // Directive
      if(tokenIdentComp(".text",ctx->token)){
	setSection(ctx,SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR);
	mode_text = true;
	mode_data = false;
	ctx->token=ctx->token->next;
	continue;
      }

      if(tokenIdentComp(".data",ctx->token)){
	setSection(ctx,SHT_PROGBITS,SHF_ALLOC|SHF_WRITE);
	mode_text = false;
	mode_data = true;
	ctx->token = ctx->token->next;
	continue;
      }

      if(mode_text){
	ctx->token = ctx->token->next;
	continue;
      }

      if(mode_data){
	ctx->token = ctx->token->next;
	continue;
      }

    }

    compError("Unexpected Token in Main Switch",ctx->token);
  }
}


void comp(char*inputfilename,char*outputfilename){
  // Create CompContext and tokenize File
  CompContext*ctx = malloc(sizeof(CompContext));
  ctx->tokenHead = tokenizeFile(inputfilename);
  ctx->tokenHead = pruneTokenTypes(ctx->tokenHead,Space|Comment);
  ctx->token = ctx->tokenHead;
  ctx->section = NULL;
  ctx->shnum = 2;
  // Create Undef Section
  ctx->sectionHead = calloc(sizeof(Section),1);

  // Create shstrtab
  ctx->shstrtab = malloc(sizeof(Section));
  ctx->sectionHead->next = ctx->shstrtab;
  ctx->sectionTail = ctx->shstrtab;
  ctx->shstrtab->size = 11;	// Sizeof(".shstrtab\0")
  ctx->shstrtab->index = 0;
  ctx->shstrtab->sectionIndex = 1;	// TODO Check for right value
  ctx->shstrtab->next = NULL;
  ctx->shstrtab->name_offset = 1;
  ctx->shstrtab->type = SHT_STRTAB;
  // Index Sections Pass
  ctx->pass = INDEX_SECTIONS;
  compPass(ctx);
  // Allocate shstrtab Buffer and insert its own name
  ctx->shstrtab->buff = malloc(ctx->shstrtab->size);
  memcpy(ctx->shstrtab->buff,"\0.shstrtab\0",11);
  ctx->shstrtab->index += 11;
  // Index_Buffers Pass: Create Sections and estimate Buffer Sizes
  ctx->token = ctx->tokenHead;
  ctx->pass = INDEX_BUFFERS;
  ctx->section = NULL;
  compPass(ctx);
  // Allocate remaining section Buffers 
  for(Section*sec = ctx->sectionHead->next;sec;sec=sec->next){
    if(!sec->buff)
      sec->buff = malloc(sec->size);
  }
  // Comp Pass
  ctx->token = ctx->tokenHead;
  ctx->pass = COMP;
  ctx->section = NULL;
  compPass(ctx);

  ctx->sectionHead->next->offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ctx->shnum;
  for(Section*sec = ctx->sectionHead->next;sec->next;sec=sec->next){
    sec->next->offset = sec->offset + sec->index;
  }

  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)    
    printf("Section %s\t size=%ld\t offset=%ld\n",
	ctx->shstrtab->buff+sec->name_offset,
	sec->size,
	sec->offset);

  
  export_elf(ctx,outputfilename);

}

