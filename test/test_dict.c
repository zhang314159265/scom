#include "scom/dict.h"
#include <assert.h>
#include <stdio.h>

void test_locate() {
	struct dict dict = dict_create_str_int();
	struct dict_entry* pentry = _dict_locate(&dict, (void*) "hello");
	assert(pentry);
	assert(pentry->flags == FREE);
	dict_free(&dict);
}

void test_basic() {
	struct dict dict = dict_create_str_int();
	dict_put(&dict, strdup("hello"), (void*) 2);
	dict_put(&dict, strdup("world"), (void*) 3);
	assert((int) dict_find_nomiss(&dict, "hello") == 2);
	assert((int) dict_find_nomiss(&dict, "world") == 3);
	assert(dict_find(&dict, "NOT_FOUND") == NULL);
	dict_free(&dict);
}

void test_insert_many() {
	struct dict dict = dict_create_str_int();
	for (int i = 0; i < 10000; ++i) {
		char *buf = malloc(3);
		buf[0] = (i % 26) + 'a';
		buf[1] = ((i / 26) % 26) + 'a';
		buf[2] = '\0';
		dict_put(&dict, buf, (void*) i);
	}
	assert(dict.size == 26 * 26);
	dict_free(&dict);
}

void test_foreach() {
	struct dict dict = dict_create_str_int();
	dict_put(&dict, strdup("hello"), (void*) 2);
	dict_put(&dict, strdup("world"), (void*) 3);
  int sum = 0;
  DICT_FOREACH(&dict, entry) {
    sum += (int) entry->val;
  }
  assert(sum == 5);
	dict_free(&dict);
}

int main(void) {
	test_locate();
	test_basic();
	test_insert_many();
  test_foreach();
	printf("PASS!\n");
	return 0;
}
