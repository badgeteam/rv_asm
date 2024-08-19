#pragma once

#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>

#include"token.h"
#include"elf.h"
//#include"section.h"

enum Pass{
  SHSTRTAB,
  INDEX,
  COMP,
};


/* In Strtab sections it is possible that only a part of the buffer will be used
 * due to duplacate strings.
 * Use Index to export instead of size.
 */
typedef struct Section{
  size_t name_offset;
  uint8_t*buff;
  size_t index;
  size_t sectionIndex;
  size_t size;
  size_t type,flags,addr,offset,link,entsize,addralign;
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
  Section*rela;
  size_t shnum;

}CompContext;


void comp(char*inputfilename,char*outputfilename);

