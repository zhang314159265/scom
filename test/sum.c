#include <math.h>

/*
 * A simple c file to generate elf file for testing elf_reader.
 * Don't bother using a math formula to compute the sum since perf
 * doen't matter here.
 */
int sum(int n) {
	int s = 0;
	for (int i = 1; i <= n; ++i) {
		s += i;
	}
	return s;
}

/*
 * sumsin will refer to an undefined symbol sin which need to be resolved by
 * the linker later.
 */
float sumsin(int n) {
  float ans = 0;
  for (int i = 1; i <= n; ++i) {
    ans += sin((float)n);
  }
  return ans;
}
