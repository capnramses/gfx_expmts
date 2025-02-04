/**
 * Making a minimal CUDA demo using snippets from documentation:
 *
 * https://cuda-tutorial.readthedocs.io/en/latest/tutorials/tutorial01/
 *
 * NOTES
 *
 * 1. CUDA kernels ~= shaders.
 * 2. But are written as an extension to CPP code, not in e.g. a shader language.
 *   * Declaration: A kernel is defined using the __global__ declaration specifier.
 *   * Invocation: n CUDA threads specified using a new <<<...>>>execution configuration syntax.
 */

#include <cuda.h>
#include <stdio.h>

__global__ void cuda_hello(){
    printf("Hello World from GPU!\n");
}

int main() {
    cuda_hello<<<1,1>>>(); 
    return 0;
}
