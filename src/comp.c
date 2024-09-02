
#include<string.h>
#include<elf.h>
#include<stdlib.h>

#include"comp.h"
#include"util.h"
#include"export.h"
#include"data.h"
#include"bss.h"
#include"riscv.h"

void nextTokenEnforceExistence(CompContext*ctx){
  if(!ctx->token->next)
    compError("Unexpected EOF",ctx->token);
  ctx->token = ctx->token->next;
}

void nextTokenEnforceComma(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != Comma)
    compError("Comma Expected",ctx->token);
  nextTokenEnforceExistence(ctx);
}


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
	0,
	0);

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


bool tryCompRelocation(CompContext*ctx,uint32_t type){
  struct Token*backupToken = ctx->token;
  struct Token*nameToken = NULL;
  int32_t addend = 0;
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
  else goto fail;

  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != BracketIn)
    goto fail;
  nextTokenEnforceExistence(ctx);
  if(ctx->token->type != Identifier)
    goto fail;
  nameToken = ctx->token;
  nextTokenEnforceExistence(ctx);

  if(ctx->token->type == Number){
    addend = parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }else if(ctx->token->type == Plus){
    nextTokenEnforceExistence(ctx);
    addend = parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }else if(ctx->token->type == Minus){
    nextTokenEnforceExistence(ctx);
    addend = - parseInt(ctx->token);
    nextTokenEnforceExistence(ctx);
  }

  if(ctx->token->type != BracketOut)
    goto fail;
//  ctx->token = ctx->token->next;

  // Apply Relocation
  addRelaEntry(ctx,ctx->section->index,getSymbol(ctx,nameToken),type,addend);
  return true;
fail:
  ctx->token = backupToken;
  return false;
}



void initSymbolList(CompContext*ctx){
  Symbol*sym = malloc(sizeof(Symbol));
  sym->name = "";
  sym->namesize = 1;
  sym->index = 0;
  sym->next = NULL;
  sym->value = 0;
  sym->size = 0;
  sym->type = 0;
  sym->vis = 0;
  sym->shndx = 0;
  ctx->symbolHead = sym;
  ctx->symbolTail = sym;
}

Symbol*getSymbol(CompContext*ctx, struct Token*nameToken){
  if(nameToken->type != Identifier)return NULL;
  for(Symbol*sym = ctx->symbolHead; sym; sym=sym->next)
    if(tokenIdentComp(sym->name,nameToken))
      return sym;
  return NULL;
}


void addSymbol(CompContext*ctx,struct Token*nameToken, uint32_t value, uint32_t size,
    uint32_t type, uint32_t vis, uint32_t shndx){
  if(ctx->pass == COMP)
    return;
  if(getSymbol(ctx,nameToken))
    compError("Symbol redefinition",nameToken);

  Symbol*sym = malloc(sizeof(Symbol));
  sym->index = ctx->symbolTail->index + 1;
  sym->next = NULL;
  sym->name = copyTokenContent(nameToken);
  sym->namesize = nameToken->buffTop - nameToken->buff + 1;
  sym->value = value;
  sym->size = size;
  sym->type = type;
  sym->vis = vis;
  sym->shndx = shndx;
  ctx->symbolTail->next = sym;
  ctx->symbolTail = sym;
}

void symbolPassPostIndex(CompContext*ctx){
  for(Symbol*sym = ctx->symbolHead; sym; sym = sym->next){
    ctx->symtab->size += sizeof(Elf32_Sym);
    ctx->symtab->shdr.sh_info++;
    ctx->strtab->size += sym->namesize;
  }
}

void symbolPassPostComp(CompContext*ctx){
  Elf32_Sym*elfsym;
  for(Symbol*sym = ctx->symbolHead; sym; sym = sym->next){
    elfsym = (Elf32_Sym*)(ctx->symtab->buff + ctx->symtab->index);
    elfsym->st_name = ctx->strtab->index;
    elfsym->st_value = sym->value;
    elfsym->st_size = sym->size;
    elfsym->st_info = sym->type;
    elfsym->st_other = sym->vis;
    elfsym->st_shndx = sym->shndx;
    ctx->symtab->index += sizeof(Elf32_Sym);
    // Insert Name into Strtab
    for(int i = 0;i<sym->namesize;i++){
      ctx->strtab->buff[ctx->strtab->index] = sym->name[i];
      ctx->strtab->index++;
    }
    ctx->strtab->buff[ctx->strtab->index] = '\0';
    ctx->strtab->index++;
  }
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
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".text",SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,0,0,4096,0x000C);
      ctx->token=ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".data",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".data",SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,0x0005);
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".rodata",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".rodata",SHT_PROGBITS,SHF_ALLOC,0,0,0,4096,0x0005);
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".bss",ctx->token)){
      if(!(ctx->section = getSectionByIdentifier(ctx)))
	ctx->section = addSection(ctx,".bss",SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,0,0,4096,0x0006);
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
      addSymbol(ctx,ctx->token,ctx->section->index,0,STT_NOTYPE,STV_DEFAULT,ctx->section->sectionIndex);
      ctx->token = ctx->token->next->next;
      continue;
    }


    if(tokenIdentComp(".equ",ctx->token)){
      
    }

    if(tokenIdentComp(".align",ctx->token)){
      nextTokenEnforceExistence(ctx);
      if(ctx->pass==INDEX){
	ctx->section->size = align(ctx->section->size, parseUImm(ctx->token, 5));
      }else{
	uint32_t align_mask = (1 << parseUImm(ctx->token, 5)) -1;
	while(ctx->section->index & align_mask){
	  ctx->section->buff[ctx->section->index] = 0;
	  ctx->section->index ++;
	}
      }
      ctx->token = ctx->token->next;
    }

    // Data
    if(ctx->section->mode & DATA)
      if(compData(ctx))
	continue;

    if(ctx->section->mode & BSS)
      if(compBSS(ctx))
	continue;

    if(ctx->section->mode & TEXT)
      if(compRV(ctx))
	continue;

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

  initSymbolList(ctx);

  // Index_Buffers Pass: Create Sections and estimate Buffer Sizes
  ctx->pass = INDEX;
  compPass(ctx);

  symbolPassPostIndex(ctx);

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

  symbolPassPostComp(ctx);

  // Set Section Offset, Size
  for(Section*sec = ctx->sectionHead->next;sec;sec=sec->next)
    sec->shdr.sh_size = sec->index;

  ctx->sectionHead->next->shdr.sh_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ctx->shnum;
  for(Section*sec = ctx->sectionHead->next;sec->next;sec=sec->next){
    sec->next->shdr.sh_offset = sec->shdr.sh_offset + (sec->buff ? sec->index : 0);
  }

  // Export
  export_elf(ctx,outputfilename);

}

