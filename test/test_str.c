#include "scom/str.h"
#include "scom/util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_no_malloc_for_capa_0() {
	struct str s = str_create(0);
	assert(s.buf == NULL);
}

void test_append() {
	struct str s = str_create(0);
	str_append(&s, 's');
	str_append(&s, 't');
	str_append(&s, 'r');
	str_append(&s, '\0');
	assert(s.len == 4);
	assert(s.capacity >= s.len);
	assert(strcmp(s.buf, "str") == 0);
	str_free(&s);
	assert(s.buf == NULL);
}

void test_concat() {
	struct str s = str_create(0);
	int r = str_concat(&s, "hello");
	assert(r == 0);
	r = str_concat(&s, "world");
	assert(r == 6);
	char buf[] = "hello\0world\0";
	assert(s.len == 12);
	for (int i = 0; i < s.len; ++i) {
		assert(s.buf[i] == buf[i]);
	}
	str_free(&s);
}

void test_nappend() {
	struct str s = str_create(0);

	str_nappend(&s, 5, 'a');
	assert(s.len == 5);
	for (int i = 0; i < 5; ++i) {
		assert(s.buf[i] == 'a');
	}
	str_free(&s);
}

void test_move() {
	struct str s = str_create(0);
	str_concat(&s, "hello");
	struct str s2 = str_move(&s);
	assert(s.buf == NULL);
	assert(s2.len == 6);
	assert(strcmp(s2.buf, "hello") == 0);
	str_free(&s2);
	// no need to free s any more
}

int main(void) {
	test_no_malloc_for_capa_0();
	test_append();
	test_concat();
	test_nappend();
	test_move();
	printf("PASS!\n");
	return 0;
}
