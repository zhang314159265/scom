#include <stdio.h>
#include "scom/elf_writer.h"

void test_create_and_free() {
	struct elf_writer elfw = elfw_create();
	elfw_free(&elfw);
}

int main(void) {
	test_create_and_free();
	printf("PASS!\n");
	return 0;
}
