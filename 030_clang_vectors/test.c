#include "apg_maths_clang.h"

int main () {
	vec3 a = {1.0f, 2.0f, 3.0f};
	vec3 b = {10.0f, 10.0f, 10.0f};
	vec3 c = a + b;
	print_vec3 (c);
	return 0;
}
