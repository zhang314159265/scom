#pragma once

#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "scom/util.h"
#include "scom/elf.h"
#include "scom/dict.h"
#include "scom/vec.h"
#include "scom/check.h"

struct elf_reader {
  char *buf; // the buffer storing the whole file content.
	           // may be updated during relocation. The memory is owned by
						 // the elf reader object.
	int file_size; // file or buf size in bytes
  Elf32_Ehdr* ehdr; // point to the beginning of buf

  Elf32_Shdr* shtab; // section header table point to buf
  int shtab_size; // number of sections

  Elf32_Shdr* sh_shstrtab; // section header for string table for section names
  char *shstrtab; // the buffer for section header string table.

  Elf32_Sym* symtab; // content of the main symtab section. Usually named '.symtab'.
  int symtab_size;
  char *symstr; // content of the string table for .symtab. Usually named '.strtab'

	/*
	 * Map a section name to the absolute address the section gonna
	 * be loaded at when the linked program is loaded by the operating
	 * system.
	 */
	struct dict section_name_to_abs_addr;
};

/*
 * Do some verification on the elf header.
 */
static void elfr_verify_ehdr(struct elf_reader* elfr) {
  // verify the magic number
  Elf32_Ehdr* ehdr = elfr->ehdr;
  assert(ehdr->e_ident[0] == 0x7F
    && ehdr->e_ident[1] == 'E'
    && ehdr->e_ident[2] == 'L'
    && ehdr->e_ident[3] == 'F');

	// only support ELF32 for now
	assert(ehdr->e_ident[EI_CLASS] == ELFCLASS32);

  assert(ehdr->e_shnum >= 0);
  assert(sizeof(Elf32_Shdr) == ehdr->e_shentsize);

	assert(ehdr->e_phnum >= 0);
	if (ehdr->e_phentsize == 0) {
		assert(ehdr->e_phnum == 0);
  } else {
		assert(sizeof(Elf32_Phdr) == ehdr->e_phentsize);
  }
}

/*
 * Load a range from the elf file. Make sure the nothing go out of range.
 */
static void *elfr_load_range(struct elf_reader* elfr, int start, int size) {
  if (size == 0) {
    return NULL;
  }
  assert(size > 0);
  assert(start >= 0 && start + size <= elfr->file_size);
  return elfr->buf + start;
}

/*
 * Get the address of section header for the given index. Fatal if the index
 * is out of range.
 */
static Elf32_Shdr* elfr_get_shdr(struct elf_reader* reader, int shidx) {
  assert(shidx >= 0 && shidx < reader->shtab_size);
  return reader->shtab + shidx;
}

/*
 * Return NULL if no section found with the name.
 */
static Elf32_Shdr* elfr_get_shdr_by_name(struct elf_reader* reader, const char* target_name) {
  for (Elf32_Shdr* shdr = reader->shtab; shdr < reader->shtab + reader->shtab_size; ++shdr) {
    if (strcmp(reader->shstrtab + shdr->sh_name, target_name) == 0) {
      return shdr;
    }
  }
  return NULL;
}

static const char *_symtype_to_str(int type) {
  switch (type) {
  case STT_NOTYPE:
    return "STT_NOTYPE";
  case STT_OBJECT:
    return "STT_OBJECT";
  case STT_FUNC:
    return "STT_FUNC";
  case STT_SECTION:
    return "STT_SECTION";
  case STT_FILE:
    return "STT_FILE";
  case STT_TLS:
    return "STT_TLS";
  case STT_GNU_IFUNC:
    return "STT_GNU_IFUNC";
  default:
    FAIL("Unrecognized symbol type %d", type);
  }
}

static const char *_symbind_to_str(int bind) {
  switch (bind) {
  case STB_LOCAL:
    return "STB_LOCAL";
  case STB_GLOBAL:
    return "STB_GLOBAL";
  case STB_WEAK:
    return "STB_WEAK";
  default:
    FAIL("Unrecognized symbol bind %d", bind);
  }
}

/*
 * Convert section header type to a string.
 */
static const char* _sht_to_str(int sht) {
  switch (sht) {
  case SHT_NULL:
    return "SHT_NULL";
  case SHT_PROGBITS:
    return "SHT_PROGBIGS";
  case SHT_SYMTAB:
    return "SHT_SYMTAB";
  case SHT_STRTAB:
    return "SHT_STRTAB";
  case SHT_NOBITS:
    return "SHT_NOBITS";
  case SHT_REL:
    return "SHT_REL";
  case SHT_GROUP:
    return "SHT_GROUP";
  default:
    FAIL("Unrecognized section header type %d", sht);
  }
}

/*
 * Convert section header number to a name.
 * Not thread safe because a static buffer is used.
 */
static const char *_shn_to_str(int shn, int nsec) {
  static char buf[256];
  switch (shn) {
  case SHN_UNDEF:
    return "SHN_UNDEF";
  case SHN_ABS:
    return "SHN_ABS";
  default:
    CHECK(shn > 0 && shn < nsec, "Unrecognized section number %d", shn);
    snprintf(buf, sizeof(buf), "%d", shn);
    return buf;
  }
}

static void elfr_dump_symbol(struct elf_reader* reader, Elf32_Sym* sym) {
  int type = ELF32_ST_TYPE(sym->st_info);
  int bind = ELF32_ST_BIND(sym->st_info);
  printf("  name '%s' type %s bind %s shndx %s\n",
    reader->symstr + sym->st_name,
    _symtype_to_str(type),
    _symbind_to_str(bind),
    _shn_to_str(sym->st_shndx, reader->shtab_size)
  );
}

/*
 * The created elf_reader takes the ownership of 'buf'.
 */
static struct elf_reader elfr_create_from_buffer(const char* buf, int size) {
  struct elf_reader reader = {0};
  reader.file_size = size;
  reader.buf = (char *) buf;
  // verify elf header
  assert(reader.file_size >= sizeof(Elf32_Ehdr));
  reader.ehdr = (void*) reader.buf;
	Elf32_Ehdr* ehdr = reader.ehdr;

	elfr_verify_ehdr(&reader);
	reader.shtab_size = ehdr->e_shnum;
	reader.shtab = elfr_load_range(&reader, reader.ehdr->e_shoff, reader.shtab_size * sizeof(Elf32_Shdr));

  // set shstrtab
  reader.sh_shstrtab = elfr_get_shdr(&reader, reader.ehdr->e_shstrndx);
	reader.shstrtab = elfr_load_range(&reader, reader.sh_shstrtab->sh_offset, reader.sh_shstrtab->sh_size);

	reader.section_name_to_abs_addr = dict_create_str_int();

	for (int i = 0; i < reader.shtab_size; ++i) {
    Elf32_Shdr* shdr = elfr_get_shdr(&reader, i);
    Elf32_Shdr* shdr_link = NULL;
    switch (shdr->sh_type) {
    case SHT_SYMTAB:
      assert(!reader.symtab && "Assume a single symtab for now");
      reader.symtab = elfr_load_range(&reader, shdr->sh_offset, shdr->sh_size);
      assert(shdr->sh_size % sizeof(Elf32_Sym) == 0);
      reader.symtab_size = shdr->sh_size / sizeof(Elf32_Sym);
      shdr_link = elfr_get_shdr(&reader, shdr->sh_link);
      assert(shdr_link->sh_type == SHT_STRTAB);
      reader.symstr = elfr_load_range(&reader, shdr_link->sh_offset, shdr_link->sh_size);
      break;
    default:
      break;
    }
	}

	// We require .symtab and it's string table for now
	assert(reader.symtab != NULL);
	assert(reader.symstr != NULL);
  return reader;
}

static struct elf_reader elfr_create(const char* path) {
  printf("Create elf reader for %s\n", path);

  struct stat elf_st;
  int status = stat(path, &elf_st);
  assert(status == 0);
  int file_size = elf_st.st_size;
  char *buf = malloc(file_size);
  FILE* fp = fopen(path, "rb");
  assert(fp);
  status = fread(buf, 1, file_size, fp);
  assert(status == file_size);
  fclose(fp);

  return elfr_create_from_buffer(buf, file_size);
}

static void elfr_free(struct elf_reader* reader) {
	assert(reader->buf);
	free(reader->buf);
	reader->buf = NULL;
	dict_free(&reader->section_name_to_abs_addr);
}

/*
 * Dump the section header table for debugging.
 */
static void elfr_dump_shtab(struct elf_reader* reader) {
  printf("The file has %d sections\n", reader->shtab_size);
  for (int i = 0; i < reader->shtab_size; ++i) {
    Elf32_Shdr* shdr = elfr_get_shdr(reader, i);
		// TODO check for out of range access
    char *name = reader->shstrtab + shdr->sh_name;
    printf("  %d: '%s' %s size %d link %d\n",
      i, name,
      _sht_to_str(shdr->sh_type),
      shdr->sh_size,
      shdr->sh_link
    );
  }
}

/*
 * Dump the symbol table for debugging.
 */
static void elfr_dump_symtab(struct elf_reader* reader) {
  printf("The file contains %d symbols\n", reader->symtab_size);
  for (int i = 0; i < reader->symtab_size; ++i) {
    Elf32_Sym* sym = reader->symtab + i;
		elfr_dump_symbol(reader, sym);
  }
}

/*
 * Check whether the section number represents a defined section.
 */
static int elfr_is_defined_section(struct elf_reader* reader, int secno) {
  return (secno > 0 && secno < reader->shtab_size) || secno == SHN_ABS;
}

/*
 * Return the list of global symbols defined in this elf file.
 * This is basically the symbols that this elf file define and can be used to
 * resolve undefined symbols in other elf files during linking.
 *
 * Both global and weak symbols are returned. I.e., this function treats a 'weak'
 * symbol as 'global'.
 *
 * The caller is responsible to free the returned vec. But the string elements
 * stored in the vec does not need to be freed.
 */
static struct vec elfr_get_global_defined_syms(struct elf_reader* reader) {
  struct vec names = vec_create(sizeof(char*));
  for (int i = 0; i < reader->symtab_size; ++i) {
    Elf32_Sym* sym = reader->symtab + i;
    int bind = ELF32_ST_BIND(sym->st_info);
    char* name = reader->symstr + sym->st_name;
    if ((bind == STB_GLOBAL || bind == STB_WEAK) && elfr_is_defined_section(reader, sym->st_shndx)) {
      vec_append(&names, &name);
    }
  }

  return names;
}

/*
 * Return the list of undefined global symbols referred by this elf file.
 * These are the symbols that should be defined by other elf files.
 *
 * The caller is responsible to free the returned vec. But the string elements
 * stored in the vec does not need to be freed.
 */
static struct vec elfr_get_undefined_syms(struct elf_reader* reader) {
  struct vec names = vec_create(sizeof(char*));
  for (int i = 0; i < reader->symtab_size; ++i) {
    Elf32_Sym* sym = reader->symtab + i;
    int bind = ELF32_ST_BIND(sym->st_info);
    char* name = reader->symstr + sym->st_name;
    if (bind == STB_GLOBAL && sym->st_shndx == 0) {
      vec_append(&names, &name);
    }
  }

  return names;
}
