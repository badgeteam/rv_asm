
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

void compSection(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != Identifier)
    compError("Identifier Expected",ctx->token);
  Token*nameToken = ctx->token;

  // Select Section if it exists
  if((ctx->section = getSectionByIdentifier(ctx))){
    // Move to EOL
    while(ctx->token->next && ctx->token->next->type != Newline)
      ctx->token = ctx->token->next;
    ctx->token = ctx->token->next;
    return;
  }

  // Create Section otherwise
  uint32_t section_flags = 0, section_type = 0;

  if(ctx->token->next && ctx->token->next->type == Comma){
    nextTokenEnforceComma(ctx);
    if(ctx->token->type != String)
      compError("String containing SHF Flags expected",ctx->token);
    for(char*cp = ctx->token->buff + 1; cp + 1 < ctx->token->buffTop; cp++){
      switch(*cp){
	case'a': section_flags |= SHF_ALLOC; break;
	case'w': section_flags |= SHF_WRITE; break;
	case'x': section_flags |= SHF_EXECINSTR; break;
	default: compError("Section Flags must be any combination of a, w, x",ctx->token);
      }
    }

    if(ctx->token->next && ctx->token->next->type == Comma){
      nextTokenEnforceComma(ctx);
      if(tokenIdentComp("@progbits",ctx->token))
	section_type = SHT_PROGBITS;
      else if(tokenIdentComp("@nobits",ctx->token))
	section_type = SHT_NOBITS;
      else compError("Section Type must be either @progbits or @nobits",ctx->token);
    }

  }
  nextTokenEnforceNewlineEOF(ctx);

  // Fill in Defaults
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
    else compError(
      "Sections whoose name does not begin with .text, .data, .rodata or .bss must have flags and type specified",
      ctx->token);
  }

    ctx->section = addSection(ctx,copyTokenContent(nameToken),section_type, section_flags, 0,0,0,4096);

}

bool tryCompSectionDirectives(CompContext*ctx){
  if(tokenIdentComp(".section",ctx->token)){
    compSection(ctx);
    return true;
  }

  if(tokenIdentComp(".text",ctx->token)){
    if(!(ctx->section = getSectionByIdentifier(ctx)))
      ctx->section = addSection(ctx,".text",SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,0,0,4096);
  }
  else if(tokenIdentComp(".data",ctx->token)){
    if(!(ctx->section = getSectionByIdentifier(ctx)))
      ctx->section = addSection(ctx,".data",SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096);
  }
  else if(tokenIdentComp(".rodata",ctx->token)){
    if(!(ctx->section = getSectionByIdentifier(ctx)))
      ctx->section = addSection(ctx,".rodata",SHT_PROGBITS,SHF_ALLOC,0,0,0,4096);
  }
  else if(tokenIdentComp(".bss",ctx->token)){
    if(!(ctx->section = getSectionByIdentifier(ctx)))
      ctx->section = addSection(ctx,".bss",SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096);
  }
  else return false;
  nextTokenEnforceNewlineEOF(ctx);
  return true;

}
