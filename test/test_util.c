#include "scom/util.h"
#include <assert.h>
#include <stdio.h>

void test_make_align() {
	assert(make_align(9, 4) == 12);
}

void test_endian_swap() {
	assert(endian_swap(0x01020304) == 0x04030201);
}

void test_string_prefix_suffix() {
	assert(startswith("abcd", "abc"));
	assert(!startswith("abcd", "abd"));
	assert(endswith("abcd", "bcd"));
	assert(!endswith("abcd", "acd"));
}

int main(void) {
	test_make_align();
	test_endian_swap();
	test_string_prefix_suffix();
	printf("PASS!\n");
	return 0;
}
