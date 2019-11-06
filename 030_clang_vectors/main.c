// http://clang.llvm.org/docs/LanguageExtensions.html#vectors-and-extended-vectors
/*
note, can check for #ifdef __clang__

so clang supports OpenCL vectors as well as GCC AltiVec (lame -- too verbose)
and NEON (nothing pops at me)
OpenCL is prob the closest to GLSL as it has V.xyzw etc.
can also swizzle with V.yx etc.
can check for this with __has_extension(attribute_ext_vector_type)

vector literals can be use like constructors are in c++
either parentehses or braces must be used
parentheses style must have one scalar element, or an exact match to the vector size
brackets have any number (less safe)

IMPORTANT!!
WARNING: PARENTHESES DID NOT WORK AS DOCUMENTED. THEY JUST ADDED TOGETHER
INTO A SCALAR. EXTREMELY BAD.

Operators supported by OpenCL vectors in Clang are:
[]
unary + - ++ --
+ - * / %
bitwise & | ^ ~
>> <<
! && ||
== != > < >= <=
=
:?
sizeof
C-style cast
reinterpret_cast
static_cast
const_cast NOT supported

SO: what can I removed from apg_maths.h?
* vector/matrix/quaternion structs, typedefs, construction functions
* vector addition/sub/mult/div
SO: what do I need to add?
* requirement for clang, not gcc
* dot and cross product functions

*/

#include <stdio.h>

typedef float vec4 __attribute__((ext_vector_type(4))); // OpenCL style
typedef float vec3 __attribute__((ext_vector_type(3)));

vec4 foo (vec3 a, float b) {
	vec4 c;
	c.xyz = a;
	c.w = b;
	return c;
}

int main () {
	vec4 a = (1.0,2.0,3.0,4.0); // parentheses must have 1 element or exact match
	printf ("vector ext test\n");
	// NOTE: need the typecast when passes as param or compile err
	a = foo ((vec3){5.0f,6.0f, 7.0f}, 8.0f);
	printf ("%f %f %f %f\n", a.x, a.y, a.z, a.w);
	vec4 b = (vec4){2,2,2,4};
	vec4 c = a + b;
	printf ("%f %f %f %f\n", c.x, c.y, c.z, c.w);

}
