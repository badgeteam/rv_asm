
#include"token.h"
#include"util.h"
#include<string.h>

void openFileError(char*filename){
  fprintf(stderr,"Error: Unable to open File %s\n",filename);
  exit(-1);
}
void duplicateFileError(char*filename){
  fprintf(stderr,"Error: Recursively including File %s\n",filename);
  exit(-1);
}
void tokenizeError(char*msg, struct File*file, uint32_t line){
  fprintf(stderr,"Error: Unable to create token in File %s Line %d\n%s\n",file->filename,line,msg);
  exit(-1);
}
/* head element of File list */
struct File*files = NULL;

/* This function reads the content of a file into a struct File*
 * @param filename Has to be zero terminated and should not be deallocated.
 */
struct File*openFile(char*filename){
  // Look for duplicate File
  for(struct File*f = files;f;f=f->next)
    if(strcmp(filename,f->filename)==0)
      duplicateFileError(filename);
  // Create File object and read file into memory
  FILE*fp = fopen(filename,"r");
  if(!fp)openFileError(filename);
  struct File*file = malloc(sizeof(struct File));
  file->filename = filename;
  fseek(fp,0L,SEEK_END);
  file->size = ftell(fp);
  fseek(fp,0L,SEEK_SET);
  file->buff = malloc(file->size);
  fread(file->buff,1,file->size,fp);
  fclose(fp);
  //Insert file into linked list files
  file->next = files;
  files = file;
  return file;
}

struct Token*tokenizeFile(char*filename){
  struct File*file = openFile(filename);
  char*buff = file->buff;
  size_t size = file->size;

  struct Token*head = NULL,*current=NULL,*last = NULL;

  uint32_t line = 1;
  size_t index = 0;
  char c;
  char*cp;
  enum TokType type;


  while(index<size){
    
    cp = buff+index;
    c = buff[index];
    

    switch(c){
      case',':
	      index++;
	      type = Colon;
	      break;
      case':':
	      index++;
	      type = Doubledot;
	      break;
      case'(':
	      index++;
	      type = BracketIn;
	      break;
      case')':
	      index++;
	      type = BracketOut;
	      break;
      case'%':
	      index++;
	      type = Percent;
	      break;
      // Character
      case'\'':
	      index++;
	      if(index >= size)
		tokenizeError("Unexpected EOF in Char",file,line);
	      if(buff[index] == '\\'){
		index++;
		if(index >= size)
		  tokenizeError("Unexpected EOF in Char",file,line);
	      }
	      index++;
	      if(index >= size)
		tokenizeError("Unexpected EOF in Char",file,line);
	      if(buff[index] != '\'')
		tokenizeError("Char not terminated with '",file,line);
	      index++;
	      type = Char;
	      break;
      // String
      case'"':
	      index++;
	      while(true){
		if(index>=size)
		  tokenizeError("Unexpected EOF in String",file,line);
		if(buff[index] == '"')
		  break;
		if(buff[index] == '\\'){
		  index++;
		  if(index>=size)
		    tokenizeError("Unexpected EOF in String",file,line);
		}
		index++;
	      }
	      index++;
	      type = String;
	      break;
      // Number
      case'-':
	      index++;
	      if(index >= size)
		tokenizeError("Unexpected EOF",file,line);
	      if(buff[index] < '0' || '9' < buff[index])
		tokenizeError("There must be a number after a Minus",file,line);
      case'0':
	      if(index + 1 < size){
		index++;
		if(buff[index] == 'x'){
		  do index++;
		  while(isHexChar(buff[index]));
		  type = Number;
		  break;
		}
	      }
      case'1'...'9':
	      do index++;
	      while(index<size && buff[index]>='0' && buff[index]<='9');
	      type = Number;
	      break;
      // Identifier or file include
      case'.':
	      if(index+8<size && StrCmp(".include",buff+index,buff+index+8)){
		// file include
		index+=8;
		while(index<size && (buff[index]==' ' || buff[index]=='\t'))
		  index++;
		if(index>=size)
		  tokenizeError("Unexpected EOF after .include",file,line);
		if(buff[index]!='"')
		  tokenizeError("Quotation mark expected after .include",file,line);
		index++;
		cp=buff+index;
		while(index<size && buff[index]!='"')
		  index++;
		if(index>=size)
		  tokenizeError("Unexpected EOF in Filename",file,line);
		buff[index]='\0';
		index++;
		current = tokenizeFile(cp);
		if(last)
		  last->next=current;
		else
		  head = current;
		current->prev = last;
		while(current->next)
		  current=current->next;
		last = current;
		continue;
	      }
	      // Identifier
      case'a'...'z':
      case'A'...'Z':
      case'_':
	      do index++;
	      while(index<size && isIdentChar(buff[index]));
	      type = Identifier;
	      break;
      // Space
      case' ':
      case'\t':
	      do index++;
	      while(index<size && (buff[index]==' ' || buff[index]=='\t'));
	      continue;
      // Comment
      case'#':
	      do index++;
	      while(index<size && buff[index] != '\n');
	      continue;
      // Newline
      case'\n':
	      type = Newline;
	      line++;
	      index++;
	      break;
      default:
	      tokenizeError("Unknown Char",file,line);
    }
    

    current = malloc(sizeof(struct Token));
    if(last)
      last->next = current;
    else
      head = current; 
    current->prev = last;
    current->line = line;
    current->file = file;
    current->next = NULL;
    current->buff = cp;
    current->buffTop = buff + index;
    current->type = type;
    last = current;
  }
  return head;
}


bool tokenIdentComp(char*str,struct Token*token){
  if(token->type != Identifier)return false;
  char c1, c2;
  for(char*tokstr = token->buff; tokstr<token->buffTop; tokstr++){
    if(*str == '\0')return false;
    c1 = *str;
    c2 = *tokstr;
    if(c1!=c2)return false;
    str++;
  }
  return *str=='\0';
}

bool tokenIdentCompCI(char*str,struct Token*token){
  if(token->type != Identifier)return false;
  char c1, c2;
  for(char*tokstr = token->buff; tokstr<token->buffTop; tokstr++){
    if(*str == '\0')return false;
    c1 = *str;
    c2 = *tokstr;
    if('a'<=c1 && c1<='z') c1 -= 0x20;
    if('a'<=c2 && c2<='z') c2 -= 0x20;
    if(c1!=c2)return false;
    str++;
  }
  return *str=='\0';
}

uint32_t parseUInt(struct Token*token){
  if(token->type != Number) compError("Number Expected",token);
  char*cp = token->buff;
  uint32_t n = 0;
  if(*cp == '-')compError("Unsigned Integer Expected",token);
  if( *cp=='0' && cp+1<token->buffTop && *(cp+1)=='x'){
    cp+=2;
    while(cp<token->buffTop){
      n = (n<<4) + parseHexChar(*cp);
      cp++;
    }
  }else{
    while(cp<token->buffTop){
      n = n*10 + *cp - '0';
      cp++;
    }
  }
  return n;
}

int32_t parseInt(struct Token*token){
  if(token->type != Number) compError("Number Expected",token);
  char*cp = token->buff;
  int32_t n = 0;
  int32_t sign = 1;
  if(*cp=='-'){
    sign = -1;
    cp++;
  }
  if(*cp=='0' && cp+1<token->buffTop && *(cp+1)=='x'){
    cp+=2;
    while(cp<token->buffTop){
      n = (n<<4) + parseHexChar(*cp);
      cp++;
    }
  }else{
    while(cp<token->buffTop){
      n = n*10 + *cp - '0';
      cp++;
    }
  }
  return sign*n;
}

/* This Function parses an unsigned bitfield
 */
uint32_t parseUImm(struct Token*token,uint32_t length){
  uint32_t n = parseUInt(token);
  if( (n>>length) != 0 ) compError("UImm out of range",token);
  return n;
}

/* This Function parses a signed bitfield
 * @param length must be >= 1
 */
uint32_t parseImm(struct Token*token,uint32_t length){
  int32_t z = parseInt(token);
  uint32_t pos = z>=0 ? z: -z;
  if((pos >> (length-1) != 0)) compError("Imm out of range",token);
  return z & ((1<<length)-1);
}

char*tokenTypeName(struct Token*token){
  switch(token->type){
    case Colon: 	return "Colon";
    case Doubledot:	return "Doubledot";
    case BracketIn:	return "Bracket In";
    case BracketOut:	return "Bracket Out";
    case Char:		return "Char";
    case String:	return "String";
    case Number:	return "Number";
    case Identifier:	return "Identifier";
    case Newline:	return "Newline";
    default:		return "";
  }
}

void printToken(struct Token*token){
  printf("Token Type=%s\tAddress=0x%lx\tSize=0x%lx\tFile=%s\tLine=%d\tContent=",
    tokenTypeName(token),
    (size_t)token->buff,
    (size_t)(token->buffTop-token->buff),
    token->file->filename,
    token->line);
  for(char*cp = token->buff;cp<token->buffTop;cp++){
    printf("%c",*cp);
  }
  printf("\n");
}

void printTokenList(struct Token*token){
  while(token){
    printToken(token);
    token=token->next;
  }
}
