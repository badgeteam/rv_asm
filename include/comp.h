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

  RV32I = 0x0100,
  RV32M = 0x0200,
  RV32A = 0x0400,
  RV32F = 0x0800,
  RV32C = 0x1000,
  RV32Zicsr = 0x2000,
  RV32Zifincei = 0x4000,
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

typedef struct CompContext{
  struct Token*tokenHead;
  struct Token*token;
  enum Pass pass;

  Section*sectionHead;
  Section*sectionTail;
  Section*section;
  Section*shstrtab;
  Section*strtab;
  Section*symtab;
  uint32_t shnum;
  uint32_t size_shstrtab;

}CompContext;

void comp(char*inputfilename,char*outputfilename);

