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
    uint32_t type, uint32_t bind, uint32_t vis, uint32_t shndx){
  if(ctx->pass == COMP)
    return;
  // add Symbol
  Symbol*sym = malloc(sizeof(Symbol));
  sym->index = ctx->symbolTail->index + 1;
  sym->next = NULL;
  sym->name = copyTokenContent(nameToken);
  sym->namesize = nameToken->buffTop - nameToken->buff + 1;
  sym->value = value;
  sym->size = size;
  sym->type = type;
  sym->bind = bind;
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
    elfsym->st_info = ELF32_ST_INFO(sym->bind,sym->type);
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
// Creates an external Symbol
// Error upon duplicate Symbols
void compExtern(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX && getSymbol(ctx,ctx->token) != NULL)
    compError("Cannot create duplicate extern symbols",ctx->token);
  addSymbol(ctx,ctx->token,0,0,STT_NOTYPE,STB_GLOBAL,STV_DEFAULT,0);
  nextTokenEnforceNewlineEOF(ctx);
}

// Creates a Symbol if it does not exist
void compLabel(CompContext*ctx){
  if(ctx->pass == INDEX && getSymbol(ctx,ctx->token)==NULL)
    addSymbol(ctx,ctx->token,ctx->section->size,0,STT_NOTYPE,STB_LOCAL,STV_DEFAULT,ctx->section->sectionIndex);
  ctx->token = ctx->token->next;
  nextTokenEnforceNewlineEOF(ctx);
}

// Creates a Symbol or modifies the Binding
void compSymbolBind(CompContext*ctx, uint32_t bind){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(sym){
      if(sym->shndx==0)
	compError("Cannot change Binding of Extern Symbol",ctx->token);
      sym->bind = bind;
    }else{
      addSymbol(ctx,ctx->token,ctx->section->size,0,STT_NOTYPE,bind,STV_DEFAULT,ctx->section->sectionIndex);
    }
  }
  nextTokenEnforceNewlineEOF(ctx);
}

// Modifies the Type of an extisting Symbol
void compType(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(!sym)compError("Symbol not found",ctx->token);
    nextTokenEnforceComma(ctx);
    if(tokenIdentComp("@function",ctx->token))
      sym->type = STT_FUNC;
    else if(tokenIdentComp("@object",ctx->token))
      sym->type = STT_OBJECT;
    else if(tokenIdentComp("@notype",ctx->token))
      sym->type = STT_NOTYPE;
    else compError("@function or @object expected",ctx->token);
  }
  else nextTokenEnforceComma(ctx);
  nextTokenEnforceNewlineEOF(ctx);
}

// Modifies the Size of an existing Symbol
void compSize(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(!sym)compError("Symbol not found",ctx->token);
    nextTokenEnforceComma(ctx);
    sym->size = parseUInt(ctx->token);
  }
  else nextTokenEnforceComma(ctx);
  nextTokenEnforceNewlineEOF(ctx);
}

// Modifies the Visibility of an existing symbol
// Non-Standard Syntax
void compVisibility(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(!sym)compError("Symbol not found",ctx->token);
    nextTokenEnforceComma(ctx);
    if(tokenIdentComp("default",ctx->token))
      sym->vis = STV_DEFAULT;
    else if(tokenIdentComp("hidden",ctx->token))
      sym->vis = STV_HIDDEN;
    else if(tokenIdentComp("internal",ctx->token))
      sym->vis = STV_INTERNAL;
    else if(tokenIdentComp("protected",ctx->token))
      sym->vis = STV_PROTECTED;
    else compError("default, hidden, internal or protected expected",ctx->token);
  }
  else nextTokenEnforceComma(ctx);
  nextTokenEnforceNewlineEOF(ctx);
}

bool tryCompSymbolDirectives(CompContext*ctx){
  
  if(tokenIdentComp(".extern",ctx->token)){
    compExtern(ctx);
    return true;
  }

  if(ctx->section){

    if(ctx->token->type == Identifier && ctx->token->next && ctx->token->next->type == Doubledot)
      compLabel(ctx);
    else if(tokenIdentComp(".global", ctx->token))
      compSymbolBind(ctx, STB_GLOBAL);
    else if(tokenIdentComp(".globl",ctx->token))
      compSymbolBind(ctx, STB_GLOBAL);
    else if(tokenIdentComp(".local",ctx->token))
      compSymbolBind(ctx, STB_LOCAL);
    else if(tokenIdentComp(".weak", ctx->token))
      compSymbolBind(ctx, STB_WEAK);
    else if(tokenIdentComp(".type", ctx->token))
      compType(ctx);
    else if(tokenIdentComp(".size",ctx->token))
      compSize(ctx);
    else if(tokenIdentComp(".visibility",ctx->token))
      compVisibility(ctx);
    else return false;
    return true;

  }
  return false;

}



