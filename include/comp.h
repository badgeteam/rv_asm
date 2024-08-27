#pragma once

#include<stdbool.h>

#include"token.h"
#include"elf.h"
//#include"section.h"

enum Pass{
  INDEX,
  COMP,
};


/* In Strtab sections it is possible that only a part of the buffer will be used
 * due to duplacate strings.
 * Use Index to export instead of size.
 */


enum AsmMode{
  DATA  = 0x0001,
  BSS   = 0x0002,
  SYM   = 0x0004,
  TEXT  = 0x0008,
};

typedef struct Section{
  char*name;
  uint8_t*buff;
  uint32_t index;
  uint32_t sectionIndex;
  uint32_t size;

  enum AsmMode mode;
  struct Section*rela;
  Elf32_Shdr shdr;
  struct Section*next;


}Section;

typedef struct Symbol{
  char*name;
  uint32_t namesize;
  uint32_t index;
  struct Symbol*next;

  uint32_t value;
  uint32_t size;
  uint32_t type;
  uint32_t vis;
  uint32_t shndx;
}Symbol;

typedef struct CompContext{
  struct Token*tokenHead;
  struct Token*token;
  enum Pass pass;

  Symbol*symbolHead,*symbolTail;

  Section*sectionHead;
  Section*sectionTail;
  Section*section;
  Section*shstrtab;
  Section*strtab;
  Section*symtab;
  uint32_t shnum;
  uint32_t size_shstrtab;

}CompContext;

void addRelaEntry(CompContext*ctx,uint32_t offset, Symbol*sym, uint32_t type, int32_t addend);

Symbol*getSymbol(CompContext*ctx,struct Token*nameToken);

void addSymbol(CompContext*ctx,struct Token*nameToken, uint32_t value, uint32_t size, uint32_t type, uint32_t vis, uint32_t shndx);


void comp(char*inputfilename,char*outputfilename);

