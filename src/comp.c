
#include<string.h>
#include<elf.h>

#include"comp.h"
#include"util.h"
#include"export.h"

/* This Function sets the current section to assemble into.
 * If the section does not exist it gets created.
 */
void setSection(CompContext*ctx,uint32_t type,uint32_t flags){
  if(ctx->pass == SHSTRTAB){
    ctx->shstrtab->size += ctx->token->buffTop - ctx->token->buff + 1;
    return;
  }

  Section*sec = ctx->shstrtab;
  // Try to select existing section
  while(sec){
    if(tokenIdentComp((char*)(ctx->shstrtab->buff + sec->name_offset), ctx->token)){
      ctx->section = sec;
      return;
    }
    sec=sec->next;
  }
  // Create New Section
  if(ctx->pass == INDEX){
    // Allocate Section
    sec = malloc(sizeof(Section));
    sec->index=0;
    sec->size=0;
    sec->type = type;
    sec->flags = flags;
    sec->buff = NULL;
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
    sec->name_offset = ctx->shstrtab->index;
    memcpy(ctx->shstrtab->buff + ctx->shstrtab->index, ctx->token->buff, ctx->token->buffTop - ctx->token->buff);
    ctx->shstrtab->index += ctx->token->buffTop - ctx->token->buff;
    ctx->shstrtab->buff[ctx->shstrtab->index] = '\0';
    ctx->shstrtab->index++;

  }
}


void initSections(CompContext*ctx){
  ctx->shnum = 4;
  ctx->sectionHead = calloc(sizeof(Section),1);
  Section*shstrtab = malloc(sizeof(Section));
  Section*strtab = malloc(sizeof(Section));
  Section*symtab = malloc(sizeof(Section));
  // Link Sections
  ctx->shstrtab = shstrtab;
  ctx->strtab = strtab;
  ctx->symtab = symtab;

  ctx->sectionHead->next = ctx->shstrtab;
  ctx->shstrtab->next = strtab;
  ctx->strtab->next = symtab;
  ctx->symtab->next = NULL;
  ctx->sectionTail = symtab;
  // Init shtrtab
  shstrtab->size = 27;
  shstrtab->index = 0;
  shstrtab->sectionIndex = 1;
  shstrtab->buff = NULL;
  shstrtab->type = SHT_STRTAB;
  shstrtab->name_offset = 1;

  strtab->name_offset = 11;
  strtab->index = 0;
  strtab->size = 0;
  strtab->buff = NULL;
  strtab->type = SHT_STRTAB;
  strtab->sectionIndex = 2;

  symtab->name_offset = 19;
  symtab->index = 0;
  symtab->size = 0;
  symtab->buff = NULL;
  symtab->type = SHT_SYMTAB;
  symtab->sectionIndex = 3;
  symtab->link = 2;


}


void compString(CompContext*ctx){
  char c;
  for(char*cp = ctx->token->buff + 1; cp<ctx->token->buffTop - 1;cp++){
    if(*cp == '\\'){
      cp++;
      if(ctx->pass == INDEX){
	ctx->section->size ++;
      }else if(ctx->pass == COMP){
	c = *cp;
	switch(c){
	  case'0':c = '\0';break;
	  case'a':c = '\a';break;
	  case'b':c = '\b';break;
	  case't':c = '\t';break;
	  case'n':c = '\n';break;
	  case'v':c = '\v';break;
	  case'f':c = '\f';break;
	  case'r':c = '\r';break;
	  case'\\':c = '\\';break;
	  case'"':c = '"';break;
	}
	ctx->section->buff[ctx->section->index] = c;
	ctx->section->index ++;
      }
    }else{
      if(ctx->pass == INDEX){
	ctx->section->size ++;
      }else if(ctx->pass == COMP){
	ctx->section->buff[ctx->section->index] = *cp;
	ctx->section->index ++;
      }
    }
  }
  ctx->token = ctx->token->next;
}


bool compData(CompContext*ctx){
  if(tokenIdentComp(".ascii",ctx->token)){
    ctx->token = ctx->token->next;
    while(ctx->token && ctx->token->type & String){
      compString(ctx);
    }
    return true;
  }
  return false;
}


bool compBss(CompContext*ctx){
  return false;
}


void compPass(CompContext*ctx){

  ctx->token = ctx->tokenHead;
  ctx->section = NULL;

  while(ctx->token){
   

    if(ctx->token->type == Newline){
      ctx->token = ctx->token->next;
      continue;
    }

    // Directive
    if(tokenIdentComp(".text",ctx->token)){
      setSection(ctx,SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR);
      ctx->token=ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".data",ctx->token)){
      setSection(ctx,SHT_PROGBITS,SHF_ALLOC|SHF_WRITE);
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".rodata",ctx->token)){
      setSection(ctx,SHT_PROGBITS,SHF_ALLOC);
      ctx->token = ctx->token->next;
      continue;
    }

    if(tokenIdentComp(".bss",ctx->token)){
      setSection(ctx,SHT_NOBITS,SHF_ALLOC|SHF_WRITE);
      ctx->token = ctx->token->next;
      continue;
    }

    if(ctx->pass == SHSTRTAB){
      while(ctx->token && ctx->token->type != Newline)
	ctx->token = ctx->token->next;
      continue;
    }


    if(ctx->token->type == Identifier && ctx->token->next && ctx->token->next->type == Doubledot){
      // Symbol
 	if(ctx->pass == INDEX){
	  ctx->symtab->size += sizeof(Elf32_Sym);
	}else if(ctx->pass == COMP){
//	  Elf32_Sym*sym =(Elf32_Sym*)(ctx->symtab->buff + ctx->symtab->index);
//	  sym->st_name = ctx->strtab->index;
//	  sym->st_value = ctx->section->index;
//	  sym->st_size = 0;
//	  sym->st_info = 0;
	  ctx->symtab->index += sizeof(Elf32_Sym);
	}
	ctx->token = ctx->token->next->next;
	continue;
    }




    if(ctx->section){
      if(ctx->section->type == SHT_PROGBITS){
	if(ctx->section->flags == (SHF_ALLOC|SHF_EXECINSTR)){
	  // text
	}
	
	if(ctx->section->flags == (SHF_ALLOC|SHF_WRITE)){
	  // data
	  if(compData(ctx))
	    continue;
	}

	if(ctx->section->flags == SHF_ALLOC){
	  // rodata
	  if(compData(ctx))
	    continue;
	}

      }
      if(ctx->section->type == SHT_NOBITS)
	if(ctx->section->flags == (SHF_ALLOC|SHF_WRITE)){
	  // bss
	  if(compBss(ctx))
	    continue;
	}
    }

    compError("Unexpected Token in Main Switch",ctx->token);
  }
}


void comp(char*inputfilename,char*outputfilename){
  // Create CompContext
  CompContext*ctx = malloc(sizeof(CompContext));

  // Tokenize File
  ctx->tokenHead = tokenizeFile(inputfilename);

  // Init Sections
  initSections(ctx);

  // Index Sections Pass
  ctx->pass = SHSTRTAB;
  compPass(ctx);

  // Allocate shstrtab Buffer and insert its own name
  ctx->shstrtab->buff = malloc(ctx->shstrtab->size);
  memcpy(ctx->shstrtab->buff,"\0.shstrtab\0.strtab\0.symtab\0",27);
  ctx->shstrtab->index += 27;

  // Index_Buffers Pass: Create Sections and estimate Buffer Sizes
  ctx->pass = INDEX;
  compPass(ctx);

  // Allocate remaining section Buffers 
  for(Section*sec = ctx->sectionHead->next;sec;sec=sec->next){
    if(!sec->buff){
      sec->buff = malloc(sec->size);
    }
  }

  // Comp Pass
  ctx->pass = COMP;
  compPass(ctx);

  // Set Section Offset
  ctx->sectionHead->next->offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ctx->shnum;
  for(Section*sec = ctx->sectionHead->next;sec->next;sec=sec->next){
    sec->next->offset = sec->offset + sec->index;
  }

  // Print Sections
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)    
    printf("Section %s\t size=%ld\t offset=%ld\n",
	ctx->shstrtab->buff+sec->name_offset,
	sec->size,
	sec->offset);

  // Export
  export_elf(ctx,outputfilename);

}

