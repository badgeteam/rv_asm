
#include<elf.h>
#include<stdlib.h>
#include"export.h"



void writeEhdr(size_t shnum, size_t shstrndx, FILE*fp){
  Elf32_Ehdr*ehdr = malloc(sizeof(Elf32_Ehdr));
 
  //TODO Maybe write data 4 bytes at a time
  ehdr->e_ident[EI_MAG0] = ELFMAG0;
  ehdr->e_ident[EI_MAG1] = ELFMAG1;
  ehdr->e_ident[EI_MAG2] = ELFMAG2;
  ehdr->e_ident[EI_MAG3] = ELFMAG3;
  ehdr->e_ident[EI_CLASS] = ELFCLASS32;
  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_ident[EI_VERSION] = EV_CURRENT;
  ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
  ehdr->e_ident[EI_ABIVERSION] = 0;
  ehdr->e_ident[9] = 0;
  ehdr->e_ident[10] = 0;
  ehdr->e_ident[11] = 0;
  ehdr->e_ident[12] = 0;
  ehdr->e_ident[13] = 0;
  ehdr->e_ident[14] = 0;
  ehdr->e_ident[15] = 0;

  ehdr->e_type = ET_REL;
  ehdr->e_machine = EM_RISCV;
  ehdr->e_version = EV_CURRENT;
  ehdr->e_entry = 0;
  ehdr->e_phoff = 0;
  ehdr->e_shoff = sizeof(Elf32_Ehdr);
  ehdr->e_flags = 0;
  ehdr->e_ehsize = sizeof(Elf32_Ehdr);
  ehdr->e_phentsize = sizeof(Elf32_Phdr);
  ehdr->e_phnum = 0;
  ehdr->e_shentsize = sizeof(Elf32_Shdr);
  ehdr->e_shnum = shnum;
  ehdr->e_shstrndx = shstrndx;
  fwrite(ehdr,sizeof(Elf32_Ehdr),1,fp);
}




void export_elf(CompContext*ctx,char*outputfilename){

  FILE*fp = fopen(outputfilename,"wb");
  if(!fp){
    fprintf(stderr,"Unable to write to file %s\n",outputfilename);
    exit(-1);
  }
   writeEhdr(ctx->shnum,ctx->shstrtab->sectionIndex,fp);
  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)
    fwrite(&(sec->shdr),sizeof(Elf32_Shdr),1,fp);

  for(Section*sec = ctx->sectionHead;sec;sec=sec->next)
    if(sec->buff)
      fwrite(sec->buff,sec->index,1,fp);

  fclose(fp);
}

