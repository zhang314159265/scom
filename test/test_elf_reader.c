#include <stdio.h>
#include "scom/elf_reader.h"
#include "scom/util.h"

const char* ELF_FILE_PATH = NULL;

void test_create_and_free() {
	struct elf_reader elfr = elfr_create(ELF_FILE_PATH);
	assert(elfr.buf != NULL);
	elfr_free(&elfr);
	assert(elfr.buf == NULL);
}

void test_text_section_exists() {
	struct elf_reader elfr = elfr_create(ELF_FILE_PATH);
	assert(elfr_get_shdr_by_name(&elfr, ".text") != NULL);
	assert(elfr_get_shdr_by_name(&elfr, ".some_arbitrary_name") == NULL);
	elfr_free(&elfr);
}

void test_syms2() {
	struct elf_reader elfr = elfr_create(ELF_FILE_PATH);
	struct vec names = vec_create(sizeof(char*));
	struct vec weaks = vec_create(sizeof(bool));

	elfr_get_global_defined_syms2(&elfr, &names, &weaks);
	int idx = vec_str_find(&names, "sum");
	assert(idx >= 0);
	assert(!*(bool*) vec_get_item(&weaks, idx)); // strong symbol

	idx = vec_str_find(&names, "sumsin");
	assert(idx >= 0);
	assert(*(bool*) vec_get_item(&weaks, idx)); // weak symbol

	vec_free(&names);
	vec_free(&weaks);
	elfr_free(&elfr);
}

void test_syms() {
	struct elf_reader elfr = elfr_create(ELF_FILE_PATH);
	struct vec defined_syms = elfr_get_global_defined_syms(&elfr);
	assert(vec_str_find(&defined_syms, "sum") >= 0);
	assert(vec_str_find(&defined_syms, "sumsin") >= 0);
	vec_free(&defined_syms);

	struct vec undefined_syms = elfr_get_undefined_syms(&elfr);
	printf("undefined_syms len %d\n", undefined_syms.len);
	assert(vec_str_find(&undefined_syms, "sin") >= 0);
	vec_free(&undefined_syms);
	elfr_free(&elfr);
}

int main(int argc, char** argv) {
	if (argc >= 2) {
		ELF_FILE_PATH = argv[1];
	}
	assert(ELF_FILE_PATH && "missing the elf file for testing");

	test_create_and_free();
	test_text_section_exists();
	test_syms();
	test_syms2();
	printf("PASS!\n");
	return 0;
}
