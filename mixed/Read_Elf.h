#include<stdio.h>
#include<string.h>

typedef struct{
	unsigned char b[8];
}int64;

typedef struct{
	unsigned char b[4];
}int32;

typedef struct{
	unsigned char b[2];
}int16;

typedef struct{
	unsigned char b;
}int8;

typedef long long Elf64_Addr;
typedef long long Elf64_Off;
typedef long long Elf64_Xword;
typedef long long Elf64_Sxword;
typedef unsigned int Elf64_Word;
typedef unsigned int Elf64_Sword;
typedef unsigned short Elf64_Half;


#define	EI_CLASS 4
#define	EI_DATA 5
#define	EI_VERSION 6
#define	EI_OSABI 7
#define	EI_ABIVERSION 8
#define	EI_PAD 9
#define	EI_NIDENT 16

#define	SHN_UNDEF 0
#define	SHN_LOPROC 0xFF00
#define	SHN_HIPROC 0xFF1F
#define	SHN_LOOS 0xFF20
#define	SHN_HIOS 0xFF3F
#define	SHN_ABS 0xFFF1
#define	SHN_COMMON 0xFFF2


typedef struct
{
	unsigned char e_ident[16]; /* ELF identification */
	Elf64_Half e_type; /* Object file type */
	Elf64_Half e_machine; /* Machine type */
	Elf64_Word e_version; /* Object file version */
	Elf64_Addr e_entry; /* Entry point address */
	Elf64_Off e_phoff; /* Program header offset */
	Elf64_Off e_shoff; /* Section header offset */
	Elf64_Word e_flags; /* Processor-specific flags */
	Elf64_Half e_ehsize; /* ELF header size */
	Elf64_Half e_phentsize; /* Size of program header entry */
	Elf64_Half e_phnum; /* Number of program header entries */
	Elf64_Half e_shentsize; /* Size of section header entry */
	Elf64_Half e_shnum; /* Number of section header entries */
	Elf64_Half e_shstrndx; /* Section name string table index */
} Elf64_Ehdr;

typedef struct
{
	Elf64_Word sh_name; /* Section name */
	Elf64_Word sh_type; /* Section type */
	Elf64_Xword sh_flags; /* Section attributes */
	Elf64_Addr sh_addr; /* Virtual address in memory */
	Elf64_Off sh_offset; /* Offset in file */
	Elf64_Xword sh_size; /* Size of section */
	Elf64_Word sh_link; /* Link to other section */
	Elf64_Word sh_info; /* Miscellaneous information */
	Elf64_Xword sh_addralign; /* Address alignment boundary */
	Elf64_Xword sh_entsize; /* Size of entries, if section has table */
} Elf64_Shdr;

typedef struct
{
	Elf64_Word st_name; /* Symbol name */
	unsigned char st_info; /* Type and Binding attributes */
	unsigned char st_other; /* Reserved */
	Elf64_Half st_shndx; /* Section table index */
	Elf64_Addr st_value; /* Symbol value */
	Elf64_Xword st_size; /* Size of object (e.g., common) */
} Elf64_Sym;


typedef struct
{
	Elf64_Word p_type; /* Type of segment */
	Elf64_Word p_flags; /* Segment attributes */
	Elf64_Off p_offset; /* Offset in file */
	Elf64_Addr p_vaddr; /* Virtual address in memory */
	Elf64_Addr p_paddr; /* Reserved */
	Elf64_Xword p_filesz; /* Size of segment in file */
	Elf64_Xword p_memsz; /* Size of segment in memory */
	Elf64_Xword p_align; /* Alignment of segment */
} Elf64_Phdr;


void read_elf();
void read_Elf_header();
void read_elf_sections();
void read_symtable();
void read_Phdr();


//??????????????????????????????????????????
unsigned int cadr=0;

//??????????????????
unsigned int csize=0;

//????????????????????????????????????
unsigned int vadr=0;

//????????????????????????????????????????????????
unsigned int dadr = 0;

//?????????????????????????????????
unsigned long long gp=0;
unsigned int vdadr = 0; // ??????????????????????????????????????????

//????????????????????????
unsigned int dsize = 0;

//main????????????????????????
unsigned int madr=0;

//??????????????????PC
unsigned int endPC=0;

//?????????????????????
unsigned int entry=0;

FILE *file=NULL;

// elf section types
const char* section_types[12] = {"SHT_NULL", "SHT_PROGBITS", "SHT_SYMTAB", "SHT_STRTAB", "SHT_RELA", "SHT_HASH", "SHT_DYNAMIC", "SHT_NOTE",  "SHT_NOBITS", "SHT_REL", "SHT_SHLIB", "SHT_DYNSYM"};
