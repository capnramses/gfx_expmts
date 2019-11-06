//https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html
#include <stdio.h>

// note: vector_size instead of ext_vector_type
// note: vector size is in bytes rather than #comps
// note: #comps has to be a power of two (for SIMD i guess?)
// "The vector_size attribute is only applicable to integral and float scalars,
// although arrays, pointers, and function return values are allowed in
// conjunction with this construct."
// All the basic integer types can be used as base types, both as signed and as
// unsigned: char, short, int, long, long long. In addition, float and double
// can be used to build floating-point vector types.
typedef float vec4 __attribute__ ((vector_size (16)));
typedef float vec3 __attribute__ ((vector_size (16)));

/*
The types defined in this manner can be used with a subset of normal C
operations. Currently, GCC will allow using the following operators on these
types: +, -, *, /, unary minus, ^, |, &, ~, %.
*/

/*
In C vectors can be subscripted as if the vector were an array with the same
number of elements and base type.

out of bound accesses invoke undefined behavior at runtime. Warnings for out of
 bound accesses for vector subscription can be enabled with -Warray-bounds.
 
It is possible to cast from one vector type to another, provided they are of
the same size (in fact, you can also cast vectors to and from other datatypes
of the same size).

You cannot operate between vectors of different lengths or different signedness
without a cast.

A port that supports hardware vector operations, usually provides a set of
built-in functions that can be used to operate on vectors. For example, a
function to add two vectors and multiply the result by a third could look
like this:

     v4si f (v4si a, v4si b, v4si c)
     {
       v4si tmp = __builtin_addv4si (a, b);
       return __builtin_mulv4si (tmp, c);
     }
     
*/
vec4 foo (vec3 a, float b) {
	vec4 c;
	c = a;
	c[3] = b;
	return c;
}

int main () {
	vec4 a = {1.0,2.0,3.0,4.0};
	printf ("vector ext test\n");
	a = foo ((vec3){5.0f,6.0f, 7.0f}, 8.0f);
	printf ("%f %f %f %f\n", a[0], a[1], a[2], a[3]);
	vec4 c = a + (vec4){2,2,2,2};
	printf ("%f %f %f %f\n", c[0], c[1], c[2], c[3]);

}
