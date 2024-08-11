
#include"token.h"
#include"util.h"

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

void swapBytes(uint8_t*buff, uint64_t index0, uint64_t index1){
	uint8_t hanoi;
	while(index0<index1){
		hanoi = buff[index0];
		buff[index0] = buff[index1];
		buff[index1] = hanoi;
		index0++;
		index1--;
	}
}

uint64_t align(uint64_t n, uint64_t bits){
	uint64_t mask = (1<<bits)-1;
	if(n & mask){
		n -= n & mask;
		n += 1 << bits;
	}
	return n;
}

void exportFile(char*filename,uint8_t*buff,uint64_t size){
	int fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0644);
	if(!fd){
		fprintf(stderr,"Unable to write to File %s\n",filename);
		exit(-1);
	}
	write(fd,buff,size);
	close(fd);
}

uint8_t parseHexChar(char c){
	switch(c){
		case'0'...'9': return c - '0';
		case'A'...'F': return c - 'A' + 10;
		case'a'...'f': return c - 'a' + 10;
		default: return 0;
	}
}

uint64_t parseUIntFromString(char*buff,char*buffTop){
	uint64_t n = 0;
	char c;
	for(char*cp=buff;cp<buffTop;cp++){
		c = *cp;
		if('0'<=c && c<='9') n = n*10 + c - '0';
		else return n;
	}
	return n;
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
//		if(*str=='\0')return true;
		if(*str!=*buff)return false;
		str++;
		buff++;
	}
}

char*extractFilename(char*buff,char*buffTop){
	uint64_t size = 0;
	while((buff+size)<buffTop && buff[size]!=')')size++;
	char*filename = malloc(size+1);
	for(int i = 0;i<size;i++)filename[i]=buff[i];
	filename[size]='\0';
	return filename;
}


void lexError(char*msg,struct File*file,int line){
	printf("Lex Error in File %s line %d.\n%s\n",file->filename,line,msg);
	exit(-1);
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
