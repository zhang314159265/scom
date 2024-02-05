#include "scom/vec.h"
#include "scom/util.h"
#include <assert.h>

void test_nomalloc_in_creator() {
	struct vec vec = vec_create(4);
	assert(vec.data == NULL);
}

void test_read_back() {
	struct vec vec = vec_create(4);

	for (int i = 0; i < 10; ++i) {
		vec_append(&vec, &i);
	}
	int ncheck = 0;
	VEC_FOREACH_I(&vec, int, item_ptr, i) {
		assert(*item_ptr == i);
		++ncheck;
	}
	assert(ncheck == 10);
	vec_free(&vec);
}

void test_csv() {
	struct vec vec = vec_create_from_csv("hello,world");
	assert(vec.len == 2);
	assert(strcmp(*(char**) vec_get_item(&vec, 0), "hello") == 0);
	assert(strcmp(*(char**) vec_get_item(&vec, 1), "world") == 0);
	vec_free(&vec);
}

int main(void) {
	test_nomalloc_in_creator();
	test_read_back();
	test_csv();
	printf("PASS!\n");
	return 0;
}
