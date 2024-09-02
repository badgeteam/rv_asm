
#include<stdlib.h>

#include"token.h"
#include"util.h"

uint32_t align(uint32_t n, uint32_t b){
  return n;
}

bool isHexChar(char c){
  switch(c){
    case'0'...'9':
    case'a'...'f':
    case'A'...'F':
      return true;
    default:
      return false;
  }
}

bool isIdentChar(char c){
  switch(c){
    case'0'...'9':
    case'a'...'z':
    case'A'...'Z':
    case'_':
    case'.':
      return true;
    default:
      return false;
  }
}

uint8_t parseHexChar(char c){
	switch(c){
		case'0'...'9': return c - '0';
		case'A'...'F': return c - 'A' + 10;
		case'a'...'f': return c - 'a' + 10;
		default: return 0;
	}
}

char parseChar(char*cp){
  if(*cp=='\\'){
    switch(*cp+1){
      case'0':return '\0';
      case'a':return '\a';
      case'b':return '\b';
      case't':return '\t';
      case'n':return '\n';
      case'v':return '\v';
      case'f':return '\f';
      case'r':return '\r';
      case'\\':return '\\';
      case'"':return '"';
      default:return 0;
    }
  }
  return *cp;
}

bool StrCmpBegin(char*str,char*buff,char*buffTop){
	while(true){
		if(buff>=buffTop)return false;
		if(*str=='\0')return true;
		if(*str!=*buff)return false;
		str++;
		buff++;
	}
}

bool StrCmp(char*str,char*buff,char*buffTop){
	while(true){
		if(buff==buffTop && *str=='\0')return true;
		if(buff>=buffTop)return false;
		if(*str!=*buff)return false;
		str++;
		buff++;
	}
}

void compWarning(char*msg,struct Token*token){
	fprintf(stderr,"Comp Warning in file %s line %d.\n",token->file->filename,token->line);
	while(token->prev!=NULL && token->prev->type != Newline)token=token->prev;
	for(char*cp = token->buff; cp<(token->file->buff+token->file->size) && *cp!='\n'; cp++)
		fprintf(stderr,"%c",*cp);
	fprintf(stderr,"\n%s\n",msg);
}

void compError(char*msg,struct Token*token){
	fprintf(stderr,"Comp Error in file %s line %d.\n",token->file->filename,token->line);
	
	for(char*cp = token->buff; cp<token->buffTop; cp++)
		fprintf(stderr,"%c",*cp);
	fprintf(stderr,"\n");

	while(token->prev!=NULL && token->prev->type != Newline)token=token->prev;
	for(char*cp = token->buff; cp<(token->file->buff+token->file->size) && *cp!='\n'; cp++)
		fprintf(stderr,"%c",*cp);
	fprintf(stderr,"\n%s\n",msg);
	exit(-1);
}
