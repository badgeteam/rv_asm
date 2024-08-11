
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<fcntl.h>

struct File{
	char*buff,*filename;
	size_t size;
	struct File*next;
};

enum TokType{
  Colon		= 0x00000001,
  Doubledot	= 0x00000002,
  BracketIn	= 0x00000004,
  BracketOut	= 0x00000008,
  Char		= 0x00000010,
  String	= 0x00000020,
  Number	= 0x00000040,
  Identifier	= 0x00000080,
  Space		= 0x00000100,
  Comment	= 0x00000200,
  Newline	= 0x00000400,
};

struct Token{
	char*buff;
	char*buffTop;
	struct Token*next;
	struct Token*prev;
	enum TokType type;
	struct File*file;
	int line;
};


// Reads files recursively and creates a Tokenchain
struct Token*tokenizeFile(char*filename);

// Checks if the token is of type Ident and compares the strings
bool tokenIdentComp(char*str,struct Token*token);

// Checks if the token is of type Ident and compares the strings while being case insensitive
bool tokenIdentCompCI(char*str,struct Token*token);

// Parses an unsigned Integer or throws an error
uint32_t parseUInt(struct Token*token);

// Parses a signed Integer or throws an error
int32_t parseInt(struct Token*token);

// Parses an unsigned Immediate Number of a specific length or throws an error
uint32_t parseUImm(struct Token*token, uint32_t length);

// Parses a signed Immediate Number of a specific length or throws an error
uint32_t parseImm(struct Token*teken, uint32_t length);

// Returns the name of the token. For Debug purposes
char*tokenTypeName(struct Token*token);

