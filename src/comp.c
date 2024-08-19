
#include<string.h>

#include"comp.h"
#include"util.h"
#include"export.h"
#include"data.h"

Section*addSection(CompContext*ctx,char*name,uint32_t type,uint32_t flags,
    uint32_t link,uint32_t info,uint32_t entsize,uint32_t addralign, enum AsmMode mode){
  Section*sec = malloc(sizeof(Section));
  sec->name = name;
  sec->index = 0;
  sec->size = 0;
  sec->buff = NULL;
  sec->next = NULL;
  sec->mode = mode;
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
      if(!(ctx->section = getSectionByIdentifier(ctx))){
	ctx->section = addSection(ctx,".text",SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,0,0,4096,0x7F04);
	ctx->section->rela = addSection(ctx,".rela.text",SHT_RELA,0,
	    ctx->symtab->sectionIndex,ctx->section->sectionIndex,0,0,0);
      }
      ctx->token=ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".data",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx))){
	ctx->section = addSection(ctx,".data",SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,0x0005);
	ctx->section->rela = addSection(ctx,".rela.data",SHT_RELA,0,
	    ctx->symtab->sectionIndex,ctx->section->sectionIndex,0,0,0);
      }
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".rodata",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx))){
	ctx->section = addSection(ctx,".rodata",SHT_PROGBITS,SHF_ALLOC,0,0,0,4096,0x0005);
	ctx->section->rela = addSection(ctx,".rela.rodata",SHT_RELA,0,
	    ctx->symtab->sectionIndex,ctx->section->sectionIndex,0,0,0);
      }
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".bss",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx))){
	ctx->section = addSection(ctx,".bss",SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,0x0006);
	ctx->section->rela = addSection(ctx,".rela.bss",SHT_RELA,0,
	    ctx->symtab->sectionIndex,ctx->section->sectionIndex,0,0,0);
      }
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".section",ctx->token)){
      compError("directive .section not implemented yet",ctx->token);
      //TODO Implement
    }

    // Guard against Assembling while no section is selected
    if(!ctx->section)
      compError("No Section selected to assemble into",ctx->token);

    // Symbols
    if(ctx->token->type == Identifier && ctx->token->next && ctx->token->next->type == Doubledot){
      if(! (ctx->section->mode & SYM))
	compError("Symbol defenition not allowed in this section",ctx->token);
      if(ctx->pass == INDEX){
	ctx->symtab->size += sizeof(Elf32_Sym);
	ctx->strtab->size += ctx->token->buffTop - ctx->token->buff + 1;
	ctx->symtab->shdr.sh_info++;
      }
      else{
	// Insert Symbol Entry
	Elf32_Sym*sym =(Elf32_Sym*)(ctx->symtab->buff + ctx->symtab->index);
	sym->st_name = ctx->strtab->index;
	sym->st_value = ctx->section->index;
	sym->st_size = 0;
	sym->st_info = STT_NOTYPE; // TODO Implement STT Types
	sym->st_other = STV_DEFAULT; // TODO Implement STV Types
	sym->st_shndx = ctx->section->sectionIndex;
	ctx->symtab->index += sizeof(Elf32_Sym);
	// Insert Name into Strtab
	for(char*cp = ctx->token->buff;cp<ctx->token->buffTop;cp++){
	  ctx->strtab->buff[ctx->strtab->index] = *cp;
	  ctx->strtab->index++;
	}
	ctx->strtab->buff[ctx->strtab->index] = '\0';
	ctx->strtab->index++;
      }
      ctx->token = ctx->token->next->next;
      continue;
    }

    compError("Unexpected Token in Main Switch",ctx->token);
  }
}


void comp(char*inputfilename,char*outputfilename){
  // Create CompContext
  CompContext*ctx = malloc(sizeof(CompContext));
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


  // Index_Buffers Pass: Create Sections and estimate Buffer Sizes
  ctx->pass = INDEX;
  compPass(ctx);

  // Shstrtab Size
  ctx->shstrtab->size += ctx->size_shstrtab;

  // Allocate section Buffers 
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next){
    if(sec->size != 0){
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

  // Comp Pass
  ctx->pass = COMP;
  compPass(ctx);

  // Set Section Offset, Size
  for(Section*sec = ctx->sectionHead->next;sec;sec=sec->next)
    sec->shdr.sh_size = sec->index;

  ctx->sectionHead->next->shdr.sh_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ctx->shnum;
  for(Section*sec = ctx->sectionHead->next;sec->next;sec=sec->next){
    sec->next->shdr.sh_offset = sec->shdr.sh_offset + sec->index;
  }

  // Print Sections
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)    
    printf("Section %s\t name_offset = %d\tsize=%d\t offset=%d\n",
	sec->name,
	sec->shdr.sh_name,
	sec->size,
	sec->shdr.sh_offset);

  // Export
  export_elf(ctx,outputfilename);

}

