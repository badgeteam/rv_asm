#pragma once

#include<stdint.h>
#include<stdbool.h>
#include<elf.h>

typedef struct File{
	char*buff,*filename;
	uint32_t size;
	struct File*next;
}File;

typedef enum TokType{
  Comma		= 0x00000001,
  Doubledot	= 0x00000002,
  BracketIn	= 0x00000004,
  BracketOut	= 0x00000008,
  Char		= 0x00000010,
  String	= 0x00000020,
  Number	= 0x00000040,
  Identifier	= 0x00000080,
  Newline	= 0x00000100,
  Percent	= 0x00000200,
  Plus		= 0x00000400,
  Minus		= 0x00000800,
}TokType;

typedef struct Token{
	char*buff;
	char*buffTop;
	struct Token*next;
	struct Token*prev;
	enum TokType type;
	struct File*file;
	int line;
}Token;

typedef enum Pass{
  INDEX,
  COMP,
}Pass;

typedef enum AsmMode{
  DATA  = 0x0001,
  BSS   = 0x0002,
  SYM   = 0x0004,
  TEXT  = 0x0008,
}AsmMode;

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

typedef struct Constant{
  Token*nameToken,*valueToken;
  struct Constant*next;
}Constant;

typedef struct CompContext{
  struct Token*tokenHead;
  struct Token*token;
  enum Pass pass;

  Constant*constantHead;

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


