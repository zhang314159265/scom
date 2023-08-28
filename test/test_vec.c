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

int main(void) {
	test_nomalloc_in_creator();
	test_read_back();
	printf("PASS!\n");
	return 0;
}
