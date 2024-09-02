#include"symbol.h"
#include"token.h"
#include<stdlib.h>

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


