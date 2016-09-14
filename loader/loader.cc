#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

#include <vector>
#include <memory>

int main(int argc, char** argv) {

  int fd = open(argv[1], O_RDONLY);

  // Reads ELF header.
  Elf32_Ehdr ehdr;
  read(fd, &ehdr, sizeof(ehdr));

  // Reads section headers.
  std::vector<Elf32_Shdr> section_headers;
  lseek(fd, ehdr.e_shoff, SEEK_SET);
  for (int i = 0; i < ehdr.e_shnum; ++i) {
    Elf32_Shdr shdr;
    read(fd, &shdr, sizeof(shdr));
    section_headers.push_back(shdr);
  }

  // Dump section headers.
  const Elf32_Shdr& shstrtab = section_headers[ehdr.e_shstrndx];
  std::vector<char> strings(shstrtab.sh_size);
  lseek(fd, shstrtab.sh_offset, SEEK_SET);
  read(fd, &strings[0], shstrtab.sh_size);

  for (const Elf32_Shdr& shdr : section_headers) {
    printf("%s\t", &strings[shdr.sh_name]);

    switch (shdr.sh_type) {
#define DUMP_SECTION_HEADER_CASE(name) \
      case name: \
        printf("%s\t 0x%08X\t 0x%08X\t 0x%08X\n", \
            #name, shdr.sh_addr, shdr.sh_offset, shdr.sh_size); \
        break;
      DUMP_SECTION_HEADER_CASE(SHT_NULL);
      DUMP_SECTION_HEADER_CASE(SHT_PROGBITS);
      DUMP_SECTION_HEADER_CASE(SHT_SYMTAB);
      DUMP_SECTION_HEADER_CASE(SHT_STRTAB);
      DUMP_SECTION_HEADER_CASE(SHT_RELA);
      DUMP_SECTION_HEADER_CASE(SHT_HASH);
      DUMP_SECTION_HEADER_CASE(SHT_DYNAMIC);
      DUMP_SECTION_HEADER_CASE(SHT_NOTE);
      DUMP_SECTION_HEADER_CASE(SHT_NOBITS);
      DUMP_SECTION_HEADER_CASE(SHT_REL);
      DUMP_SECTION_HEADER_CASE(SHT_SHLIB);
      DUMP_SECTION_HEADER_CASE(SHT_DYNSYM);
      DUMP_SECTION_HEADER_CASE(SHT_NUM);
#undef DUMP_SECTION_HEADER_CASE
      default:
        printf("UNKNOWN: %X\n", shdr.sh_type);
    }
  }

  // Read and dump program header.
  std::vector<Elf32_Phdr> program_headers;
  lseek(fd, ehdr.e_phoff, SEEK_SET);
  for (int i = 0; i < ehdr.e_phnum; ++i) {
    Elf32_Phdr phdr;
    read(fd, &phdr, sizeof(phdr));
    program_headers.push_back(phdr);
    switch (phdr.p_type) {
#define DUMP_PROGRAM_HEADER_CASE(name) \
      case name: \
        printf("%s\t 0x%08X\t 0x%08X\t 0x%08X\t 0x%08X\t 0x%08X\n", \
               #name, phdr.p_offset, phdr.p_vaddr, phdr.p_paddr, \
               phdr.p_filesz, phdr.p_memsz); \
        break; 
      DUMP_PROGRAM_HEADER_CASE(PT_NULL);
      DUMP_PROGRAM_HEADER_CASE(PT_LOAD);
      DUMP_PROGRAM_HEADER_CASE(PT_DYNAMIC);
      DUMP_PROGRAM_HEADER_CASE(PT_INTERP);
      DUMP_PROGRAM_HEADER_CASE(PT_NOTE);
      DUMP_PROGRAM_HEADER_CASE(PT_SHLIB);
      DUMP_PROGRAM_HEADER_CASE(PT_PHDR);
      DUMP_PROGRAM_HEADER_CASE(PT_TLS);
#undef DUMP_PROGRAM_HEADER_CASE
      default:
        printf("Unknown: %X\n", phdr.p_type);
        break;
    }
  }

  // Loads PT_LOAD
  for (const Elf32_Phdr& phdr : program_headers) {
    if (phdr.p_type != PT_LOAD)
      continue;

    // Align the address
    Elf32_Addr v_addr = phdr.p_vaddr;
    int rest = phdr.p_vaddr % phdr.p_align;
    if (rest != 0) {
      v_addr -= rest;
    }
    int diff = phdr.p_vaddr - v_addr;

    Elf32_Addr offset = phdr.p_offset - diff;
    Elf32_Word filesize = phdr.p_filesz + diff;
    Elf32_Word aligned_filesize = phdr.p_filesz + diff;
    rest = filesize % phdr.p_align;
    if (rest != 0) {
      aligned_filesize += phdr.p_align - rest;
    }
    Elf32_Word memsize = phdr.p_memsz + diff;
    rest = memsize % phdr.p_align;
    if (rest != 0) {
      memsize += phdr.p_align - rest;
    }

    // map it
    mmap((void*)v_addr, aligned_filesize, PROT_READ | PROT_WRITE,
         MAP_FILE | MAP_PRIVATE | MAP_FIXED, fd, offset);

    int prot = 0;
    if (phdr.p_flags & PF_R)
      prot |= PROT_READ;
    if (phdr.p_flags & PF_W)
      prot |= PROT_WRITE;
    if (phdr.p_flags & PF_X)
      prot |= PROT_EXEC;

    if (filesize != memsize) {
      mmap((void*)(v_addr + filesize), memsize - filesize,
           prot, MAP_ANON | MAP_PRIVATE, -1, 0);
    }

    mprotect((void*)v_addr, aligned_filesize, prot);
  }

  // Dumps PT_DYNAMIC types.
  // http://docs.oracle.com/cd/E19620-01/805-5821/chapter6-42444/index.html
  for (const Elf32_Phdr& phdr : program_headers) {
    if (phdr.p_type != PT_DYNAMIC)
      continue;
    Elf32_Rel* rel_head = nullptr;
    Elf32_Sym* sym_head = nullptr;
    int rel_size = 0;
    int rel_ent = 0;
    int plt_rel_size = 0;
    char* strtab = nullptr;
    for (Elf32_Dyn* dyn = (Elf32_Dyn*)phdr.p_vaddr; dyn->d_tag != DT_NULL;
         dyn++) {
      switch (dyn->d_tag) {
        case DT_NULL:
          printf("DT_NULL\n");
          break;
        case DT_NEEDED:
          printf("DT_NEEDED\n");
          break;
        case DT_PLTRELSZ:
          printf("DT_PLTRELSZ\n");
          plt_rel_size = dyn->d_un.d_val;
          break;
        case DT_PLTGOT:
          printf("DT_PLTGOT\n");
          break;
        case DT_HASH:
          printf("DT_HASH\n");
          break;
        case DT_STRTAB:
          printf("DT_STRTAB\n");
          strtab = (char*)dyn->d_un.d_ptr;
          break;
        case DT_SYMTAB:
          printf("DT_SYMTAB\n");
          sym_head = (Elf32_Sym*)dyn->d_un.d_ptr;
          break;
        case DT_RELA:
          printf("DT_RELA\n");
          break;
        case DT_RELASZ:
          printf("DT_RELASZ\n");
          break;
        case DT_RELAENT:
          printf("DT_RELAENT\n");
          break;
        case DT_STRSZ:
          printf("DT_STRSZ\n");
          break;
        case DT_SYMENT:
          printf("DT_SYMENT\n");
          break;
        case DT_INIT:
          printf("DT_INIT\n");
          break;
        case DT_FINI:
          printf("DT_FINI\n");
          break;
        case DT_SONAME:
          printf("DT_SONAME\n");
          break;
        case DT_RPATH:
          printf("DT_RPATH\n");
          break;
        case DT_SYMBOLIC:
          printf("DT_SYMBOLIC\n");
          break;
        case DT_REL:
          printf("DT_REL\n");
          rel_head = (Elf32_Rel*)dyn->d_un.d_ptr;
          break;
        case DT_RELSZ:
          printf("DT_RELSZ\n");
          rel_size = dyn->d_un.d_val;
          break;
        case DT_RELENT:
          printf("DT_RELENT\n");
          rel_ent = dyn->d_un.d_val;
          break;
        case DT_PLTREL:
          printf("DT_PLTREL\n");
          if (dyn->d_un.d_val != DT_REL) {
            printf("Only support DT_PLTREL=REL\n");
            abort();
          }
          break;
        case DT_DEBUG:
          printf("DT_DEBUG\n");
          break;
        case DT_TEXTREL:
          printf("DT_TEXTREL\n");
          break;
        case DT_JMPREL:
          printf("DT_JMPREL\n");
          break;
        case DT_ENCODING:
          printf("DT_ENCODING\n");
          break;
        case DT_LOOS:
          printf("DT_LOOS\n");
          break;
        case DT_HIOS:
          printf("DT_HIOS\n");
          break;
        case DT_VALRNGLO:
          printf("DT_VALRNGLO\n");
          break;
        case DT_VALRNGHI:
          printf("DT_VALRNGHI\n");
          break;
        case DT_ADDRRNGLO:
          printf("DT_ADDRRNGLO\n");
          break;
        case DT_ADDRRNGHI:
          printf("DT_ADDRRNGHI\n");
          break;
        case DT_VERSYM:
          printf("DT_VERSYM\n");
          break;
        case DT_RELACOUNT:
          printf("DT_RELACOUNT\n");
          break;
        case DT_RELCOUNT:
          printf("DT_RELCOUNT\n");
          break;
        case DT_FLAGS_1:
          printf("DT_FLAGS_1\n");
          break;
        case DT_VERDEF:
          printf("DT_VERDEF\n");
          break;
        case DT_VERDEFNUM:
          printf("DT_VERDEFNUM\n");
          break;
        case DT_VERNEED:
          printf("DT_VERNEED\n");
          break;
        case DT_VERNEEDNUM:
          printf("DT_VERNEEDNUM\n");
          break;
        default:
          printf("Unknown Tag:%d\n", dyn->d_tag);
          break;
      }
    }

    int rel_count = (rel_size + plt_rel_size) / rel_ent;

    for (int i = 0; i < rel_count; ++i) {
      Elf32_Rel* rel = rel_head + i;
      Elf32_Sym* sym = sym_head + ELF32_R_SYM(rel->r_info);

      int* plt_addr = (int*)rel->r_offset;
      void* target_addr = dlsym(RTLD_DEFAULT, strtab + sym->st_name);
      int type = ELF32_R_TYPE(rel->r_info);

      printf("Relocating %s\n", strtab + sym->st_name);

      switch (type) {
        case R_386_JMP_SLOT:
          *plt_addr = (int)target_addr;
          break;
        case R_386_GLOB_DAT:
          break;
        default:
          printf("Unknown type: %d\n", type);
          abort();
      }

    }
  }

  ((void*(*)())ehdr.e_entry)();
  return 0;
}
