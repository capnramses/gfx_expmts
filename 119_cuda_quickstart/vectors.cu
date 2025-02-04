/**
 * Making a minimal CUDA demo using snippets from documentation:
 *
 * https://docs.nvidia.com/cuda/cuda-quick-start-guide/index.html
 * https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#
 * https://www.cstechera.com/2015/07/addition-of-two-numbers-in-cuda-simple.html
 *
 * NOTES
 *
 * 1. CUDA kernels ~= shaders.
 * 2. But are written as an extension to CPP code, not in e.g. a shader language.
 *   * Declaration: A kernel is defined using the __global__ declaration specifier.
 *   * Invocation: n CUDA threads specified using a new <<<...>>>execution configuration syntax.
 * 3. Note that the official docs examples didn't have the necessary malloc and memcpy to<->from device
 */

#include <cuda.h>
#include <stdio.h>

// The following sample code, using the built-in variable threadIdx, adds two vectors A and B of size N and stores the result into vector C.
__global__ void VecAdd( float* A, float* B, float* C ) {
  /*
For convenience, threadIdx is a 3-component vector, so that threads can be identified using a one-dimensional, two-dimensional, or three-dimensional thread
index, forming a one-dimensional, two-dimensional, or three-dimensional block of threads, called a thread block. This provides a natural way to invoke
computation across the elements in a domain such as a vector, matrix, or volume.

There is a limit to the number of threads per block, since all threads of a block are expected to reside on the same streaming multiprocessor core and must
share the limited memory resources of that core. On current GPUs, a thread block may contain up to 1024 threads.
  */
  int i = threadIdx.x;
  C[i]  = A[i] + B[i];
  printf( "C[%i] = %f\n", i, C[i] );
}

int main() {
  printf( "Hello CUDA\n" );

#define N 3
  float a[N] = { 1, 2, 3 };
  float b[N] = { 13, 22, 31 };
  float c[N] = { 0, 0, 0 };

  float *d_a, *d_b, *d_c; // Device variable Declaration

  // Allocation of Device Variables
  cudaMalloc( (void**)&d_a, sizeof( float ) * 3 );
  cudaMalloc( (void**)&d_b, sizeof( float ) * 3 );
  cudaMalloc( (void**)&d_c, sizeof( float ) * 3 );

  // Copy Host Memory to Device Memory
  cudaMemcpy( d_a, &a, sizeof( float ) * 3, cudaMemcpyHostToDevice );
  cudaMemcpy( d_b, &b, sizeof( float ) * 3, cudaMemcpyHostToDevice );
  cudaMemcpy( d_c, &c, sizeof( float ) * 3, cudaMemcpyHostToDevice );

  // Kernel invocation with N threads
  // The number of threads per block and the number of blocks per grid specified in the <<<...>>> syntax can be of type int or dim3. Two-dimensional blocks or
  // grids can be specified as in the examples.
  VecAdd<<<1, N>>>( d_a, d_b, d_c );

  // Copy Device Memory to Host Memory
  cudaMemcpy( &c, d_c, sizeof( float ) * 3, cudaMemcpyDeviceToHost );

  printf( "before fence result C: %f %f %f\n", c[0], c[1], c[2] );

  // Fence all work
  cudaDeviceSynchronize(); // can also call just to wait for 1 stream cudaStreamSynchronize(cudaStream)

  printf( "after frence result C: %f %f %f\n", c[0], c[1], c[2] );

  /* Another example:
// Kernel invocation with one block of N * N * 1 threads
int numBlocks = 1;
dim3 threadsPerBlock(N, N);
MatAdd<<<numBlocks, threadsPerBlock>>>(A, B, C);
  */

  printf( "Normal exit\n" );
  return 0;
}
