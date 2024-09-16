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
  Times		= 0x00001000,
  Slash         = 0x00002000,
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

typedef struct Section{
  char*name;
  uint8_t*buff;
  uint32_t index;
  uint32_t sectionIndex;
  uint32_t size;

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
  uint32_t bind;
  uint32_t vis;
  uint32_t shndx;

  bool locked;
}Symbol;

typedef struct Constant{
  Token*nameToken;
  uint32_t value;
  int sign;
  struct Constant*next;
}Constant;

typedef enum LrType{
//  Lr_ValueMask    = 0x000F,
  Lr_Number       = 0x0001,
  Lr_Constant     = 0x0002,
  Lr_Symbol       = 0x0004,
//  Lr_BracketMask  = 0x00F0,
  Lr_BracketIn    = 0x0010,
  Lr_BracketOut   = 0x0020,
//  Lr_OperandMask  = 0xFF00,
  Lr_Add          = 0x0100,
  Lr_Sub          = 0x0200,
  Lr_Mul          = 0x0400,
  Lr_Div          = 0x0800,
  Lr_Rem          = 0x1000,
}LrType;

typedef struct LrToken{
  LrType type;
  uint32_t value;
  int sign;
  Token*token;
  struct LrToken*next;
}LrToken;

typedef struct CompContext{
  struct Token*tokenHead;
  struct Token*token;
  enum Pass pass;

  Constant*constantHead;
  LrToken*lrHead;
  LrToken*lrUnused;
  uint32_t lrBracketDepth;

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


