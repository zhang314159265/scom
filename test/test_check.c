#include <scom/check.h>

void test_basic() {
	int val = 4;
	CHECK(val % 2 == 0, "val is not even: %d", val);
}

int main(void) {
	// I don't know how to test the scenario if the check fail..
	test_basic();
	printf("PASS!\n");
	return 0;
}
