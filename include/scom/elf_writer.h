#pragma once

#include "scom/str.h"
#include "scom/elf.h"
#include "scom/vec.h"
#include "scom/util.h"

// this is not necessary to be the entry point of the executable if the
// text segment does not start with the first instruction to execute.
#define EXECUTABLE_START_VA 0x50000000
#define ALIGN_BYTES 16

struct elf_writer {
	Elf32_Ehdr ehdr;
  struct str shstrtab; // contains section names

	uint32_t next_file_off; // track the layout of the elf file
	uint32_t next_va; // track the layout in memory when the elf file is loaded

	struct vec phdrtab; // program header table
	struct vec pbuftab; // program buffer table

	struct vec shdrtab; // section header table
};


static int elfw_add_shdr(struct elf_writer* writer, const char* name, uint32_t type, uint32_t flags, uint32_t link, uint32_t info, uint32_t addralign, uint32_t entsize);

static void elfw_init_ehdr(struct elf_writer* writer) {
  Elf32_Ehdr* ehdr = &writer->ehdr;
  memset(ehdr, 0, sizeof(*ehdr));
  ehdr->e_ident[0] = 0x7f;
  ehdr->e_ident[1] = 'E';
  ehdr->e_ident[2] = 'L';
  ehdr->e_ident[3] = 'F';
  ehdr->e_ident[4] = 1; // ELFCLASS32
  ehdr->e_ident[5] = 1; // little endian
  ehdr->e_ident[6] = 1; // file version == EV_CURRENT
  ehdr->e_ident[7] = 0; // OSABI
  ehdr->e_ident[8] = 0; // ABI version

  ehdr->e_type = ET_EXEC;
  ehdr->e_machine = EM_386;
  ehdr->e_version = 1;
  ehdr->e_entry = 0; // usually decided by the address of _start symbol
  ehdr->e_phoff = 0;
  ehdr->e_shoff = 0;
  ehdr->e_flags = 0;
  ehdr->e_ehsize = sizeof(Elf32_Ehdr);
  assert(sizeof(Elf32_Ehdr) == 52); // this should be a static assert

  ehdr->e_phentsize = sizeof(Elf32_Phdr);
  assert(sizeof(Elf32_Phdr) == 32);
  ehdr->e_phnum = 0;
  ehdr->e_shentsize = sizeof(Elf32_Shdr);
  assert(sizeof(Elf32_Shdr) == 40);
  ehdr->e_shnum = 0;
  ehdr->e_shstrndx = 0; // updated later
}

static struct elf_writer elfw_create() {
	struct elf_writer writer;
	elfw_init_ehdr(&writer);
	writer.shstrtab = str_create(0);
	str_append(&writer.shstrtab, 0);
	writer.phdrtab = vec_create(sizeof(Elf32_Phdr));
	writer.pbuftab = vec_create(sizeof(struct str));
	writer.shdrtab = vec_create(sizeof(Elf32_Shdr));

	// TODO: create the header for .text section
	elfw_add_shdr(&writer, NULL, 0, 0, 0, 0, 0, 0);
	writer.ehdr.e_shstrndx = elfw_add_shdr(&writer, ".shstrtab", SHT_STRTAB, 0, 0, 0, 1, 0);

	writer.next_file_off = sizeof(writer.ehdr);
	writer.next_va = EXECUTABLE_START_VA;

	return writer;
}

static void elfw_free(struct elf_writer* writer) {
	str_free(&writer->shstrtab);
	vec_free(&writer->phdrtab);
	VEC_FOREACH(&writer->pbuftab, struct str, pbufptr) {
		str_free(pbufptr);
	}
	vec_free(&writer->pbuftab);
}

static Elf32_Phdr _elfw_create_phdr(uint32_t file_off, uint32_t va, uint32_t memsize, const char* name) {
  assert(file_off % 4096 == 0);
  assert(va % 4096 == 0);
  Elf32_Phdr phdr;

  bool isbss = (strcmp(name, ".bss") == 0);

  phdr.p_type = PT_LOAD; // TODO don't hardcode
  phdr.p_offset = file_off;
  phdr.p_vaddr = va;
  phdr.p_paddr = phdr.p_vaddr;
  phdr.p_memsz = memsize;
  if (isbss) {
    phdr.p_filesz = 0;
  } else {
    phdr.p_filesz = memsize;
  }
  phdr.p_flags = PF_R;
  if (strcmp(name, ".text") == 0) {
    phdr.p_flags |= PF_X;
  }
  if (strcmp(name, ".data") == 0 || strcmp(name, ".bss") == 0) {
    phdr.p_flags |= PF_W;
  }
  phdr.p_align = ALIGN_BYTES; // TODO don't hardcode

	return phdr;
}

// this function will move writer->next_file_off
static void elfw_place_section_to_layout(struct elf_writer* writer, Elf32_Shdr* sh) {
  int align = sh->sh_addralign;
  assert(align > 0);
  int off = writer->next_file_off;
  off = make_align(off, align);
  sh->sh_offset = off;
  assert(sh->sh_size >= 0);
  writer->next_file_off = off + sh->sh_size;
}

// write the content of a section to file.
static void _elfw_write_section(FILE* fp, Elf32_Shdr* sh, void *data) {
  fseek(fp, sh->sh_offset, SEEK_SET);
  fwrite(data, 1, sh->sh_size, fp);
}

/*
 * Add a section header before all it's fields can be decided (e.g. size/offset) so
 * that the section number can be available early enough.
 *
 * The section header need to be revised before writing the file.
 *
 * Return the index of the section header just added.
 */
static int elfw_add_shdr(struct elf_writer* writer, const char* name, uint32_t type, uint32_t flags, uint32_t link, uint32_t info, uint32_t addralign, uint32_t entsize) {
  Elf32_Shdr newsh;
  memset(&newsh, 0, sizeof(Elf32_Shdr));

  int name_idx = 0;
  if (name) {
    name_idx = str_concat(&writer->shstrtab, name);
  }
  newsh.sh_name = name_idx;
  newsh.sh_type = type;
  newsh.sh_flags = flags;
  newsh.sh_link = link;
  newsh.sh_info = info;
  newsh.sh_addralign = addralign;
  newsh.sh_entsize = entsize;

  vec_append(&writer->shdrtab, &newsh);
  return writer->shdrtab.len - 1;
}
