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
  sym->locked = true;
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
    uint32_t type, uint32_t bind, uint32_t vis, uint32_t shndx, bool locked){
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
  sym->locked = locked;
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


void compLabel(CompContext*ctx){
  if(!ctx->section)
    compError("No Section selected",ctx->token);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(!sym){
      addSymbol(ctx,ctx->token,ctx->section->size,0,STT_NOTYPE,STB_LOCAL,STV_DEFAULT,ctx->section->sectionIndex, true);
    }else if(!sym->locked){
      sym->value = ctx->section->size;
      sym->shndx = ctx->section->sectionIndex;
      sym->locked = true;
    }else if(sym->value != ctx->section->size || sym->shndx != ctx->section->sectionIndex){
      compError("Cannot Redefine Symbol Value or Shndx",ctx->token);
    }
  }
  ctx->token = ctx->token->next;
  nextTokenEnforceNewlineEOF(ctx);
}

#include<stdio.h>

void compExtern(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    if(!getSymbol(ctx,ctx->token))
      addSymbol(ctx,ctx->token,0,0,STT_NOTYPE,STB_GLOBAL,STV_DEFAULT,0,false);
  }
  nextTokenEnforceNewlineEOF(ctx);
}

// Creates a Symbol or modifies the Binding
void compSymbolBind(CompContext*ctx, uint32_t bind){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(sym)
      sym->bind = bind;
    else
      addSymbol(ctx,ctx->token,0,0,STT_NOTYPE,bind,STV_DEFAULT,0,false);
  }
  nextTokenEnforceNewlineEOF(ctx);
}

void compSymbolVis(CompContext*ctx, uint32_t vis){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(sym)
      sym->vis = vis;
    else
      addSymbol(ctx,ctx->token,0,0,STT_NOTYPE,STB_LOCAL,vis,0,false);
  }
  nextTokenEnforceNewlineEOF(ctx);
}

// Modifies the Type of an extisting Symbol
void compType(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Token*nameToken = ctx->token;
    nextTokenEnforceComma(ctx);
    uint32_t type = 0;
    if(tokenIdentComp("@function",ctx->token))
      type = STT_FUNC;
    else if(tokenIdentComp("@object",ctx->token))
      type = STT_OBJECT;
    else if(tokenIdentComp("@notype",ctx->token))
      type = STT_NOTYPE;
    else compError("@function or @object expected",ctx->token);
    Symbol*sym = getSymbol(ctx,nameToken);
    if(sym)
      sym->type = type;
    else
      addSymbol(ctx,ctx->token,0,0,type,STB_LOCAL,STV_DEFAULT,0,false);
  }
  else nextTokenEnforceComma(ctx);
  nextTokenEnforceNewlineEOF(ctx);
}

// Modifies the Size of an existing Symbol
void compSize(CompContext*ctx){
  nextTokenEnforceExistence(ctx);
  if(ctx->pass == INDEX){
    Token*nameToken = ctx->token;
    nextTokenEnforceComma(ctx);
    uint32_t size = parseUInt(ctx->token);
    Symbol*sym = getSymbol(ctx,ctx->token);
    if(sym)
      sym->size = size;
    else
     addSymbol(ctx,nameToken,0,size,STT_NOTYPE,STB_LOCAL,STV_DEFAULT,0,false);
  }
  else nextTokenEnforceComma(ctx);
  nextTokenEnforceNewlineEOF(ctx);
}

bool tryCompSymbolDirectives(CompContext*ctx){
  if(ctx->token->type == Identifier && ctx->token->next && ctx->token->next->type == Doubledot)
    compLabel(ctx);

  else if(tokenIdentComp(".extern",ctx->token))
    compExtern(ctx);

  else if(tokenIdentComp(".global", ctx->token))
    compSymbolBind(ctx, STB_GLOBAL);
  else if(tokenIdentComp(".globl",ctx->token))
    compSymbolBind(ctx, STB_GLOBAL);
  else if(tokenIdentComp(".local",ctx->token))
    compSymbolBind(ctx, STB_LOCAL);
  else if(tokenIdentComp(".weak", ctx->token))
    compSymbolBind(ctx, STB_WEAK);

  else if(tokenIdentComp(".hidden_names",ctx->token))
    compSymbolVis(ctx,STV_HIDDEN);
  else if(tokenIdentComp(".internal",ctx->token))
    compSymbolVis(ctx,STV_INTERNAL);
  else if(tokenIdentComp(".protected",ctx->token))
    compSymbolVis(ctx,STV_PROTECTED);

  else if(tokenIdentComp(".type", ctx->token))
    compType(ctx);
  else if(tokenIdentComp(".size",ctx->token))
    compSize(ctx);
  else return false;
  return true;
}



