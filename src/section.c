
#include"section.h"
#include"token.h"
#include"stdlib.h"

Section*addSection(CompContext*ctx,char*name,uint32_t type,uint32_t flags,
    uint32_t link,uint32_t info,uint32_t entsize,uint32_t addralign){
  Section*sec = malloc(sizeof(Section));
  sec->name = name;
  sec->index = 0;
  sec->size = 0;
  sec->buff = NULL;
  sec->next = NULL;
  sec->sectionIndex = ctx->shnum;
  ctx->shnum++;
  sec->rela = NULL;
  sec->shdr.sh_type = type;
  sec->shdr.sh_flags = flags;
  sec->shdr.sh_link = link;
  sec->shdr.sh_info = info;
  sec->shdr.sh_entsize = entsize;
  sec->shdr.sh_addralign = addralign;
  sec->shdr.sh_addr = 0;
  // Set Later
  sec->shdr.sh_name = 0;
  sec->shdr.sh_offset = 0;
  sec->shdr.sh_size = 0;
  // Insert into List
  if(ctx->sectionTail){
    ctx->sectionTail->next = sec;
  }else{
    ctx->sectionHead = sec;
  }
  ctx->sectionTail = sec;
  // Shstrtab Buffsize
  while(*name!='\0'){
    ctx->size_shstrtab++;
    name++;
  }
  ctx->size_shstrtab++;

  return sec;
}


Section*getSectionByIdentifier(CompContext*ctx){
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)
    if(tokenIdentComp(sec->name,ctx->token))
      return sec;
  return NULL;
}


