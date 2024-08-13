
#include<string.h>

#include"comp.h"
#include"util.h"



void setSection(CompContext*ctx){
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
    sec->sectionIndex = ctx->sectionHead->sectionIndex+1;
    sec->next = NULL;
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
	setSection(ctx);
	mode_text = true;
	mode_data = false;
	ctx->token=ctx->token->next;
	continue;
      }

      if(tokenIdentComp(".data",ctx->token)){
	setSection(ctx);
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
  // Create shstrtab
  ctx->shstrtab = malloc(sizeof(Section));
  ctx->sectionHead = ctx->shstrtab;
  ctx->sectionTail = ctx->shstrtab;
  ctx->shstrtab->size = 10;	// Sizeof(".shstrtab\0")
  ctx->shstrtab->index = 0;
  ctx->shstrtab->sectionIndex = 1;	// TODO Check for right value
  ctx->shstrtab->next = NULL;
  ctx->shstrtab->name_offset = 0;
  // Index Sections Pass
  ctx->pass = INDEX_SECTIONS;
  compPass(ctx);
  // Allocate shstrtab Buffer and insert its own name
  ctx->shstrtab->buff = malloc(ctx->shstrtab->size);
  memcpy(ctx->shstrtab->buff,".shstrtab\0",10);
  ctx->shstrtab->index += 10;
  // Index_Buffers Pass: Create Sections and estimate Buffer Sizes
  ctx->token = ctx->tokenHead;
  ctx->pass = INDEX_BUFFERS;
  ctx->section = NULL;
  compPass(ctx);
  // Allocate remaining section Buffers 
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)
    if(!sec->buff)
      sec->buff = malloc(sec->size);
  // Comp Pass
  ctx->token = ctx->tokenHead;
  ctx->pass = COMP;
  ctx->section = NULL;
  compPass(ctx);

  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)    
    printf("Section %s\t size=%ld\n",ctx->shstrtab->buff+sec->name_offset, sec->size);

}

