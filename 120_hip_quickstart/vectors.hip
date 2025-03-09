/**
 * This is vectors.cu from 119, converted from CUDA to HIP.
 * 
 * hipcc vectors.hip -o vectors.bin
 */

#include <hip/hip_runtime.h>
#include <stdio.h>

// The following sample code, using the built-in variable hipThreadIdx_x,
// adds two vectors A and B of size N and stores the result into vector C.
__global__ void VecAdd( float* A, float* B, float* C ) {
  int i = hipThreadIdx_x; // threadIdx.x;
  C[i]  = A[i] + B[i];
  printf( "C[%i] = %f\n", i, C[i] );
}

int main() {
  hipDeviceProp_t devProp;
  hipGetDeviceProperties( &devProp, 0 );
  printf( "ROCm HIP System: %i.%i %s\n", devProp.major, devProp.minor, devProp.name );

#define N 3
  float a[N] = { 1, 2, 3 };
  float b[N] = { 13, 22, 31 };
  float c[N] = { 0, 0, 0 };

  float *d_a, *d_b, *d_c; // Device variable Declaration

  // Same params as cudaMalloc
  hipMalloc( (void**)&d_a, sizeof( float ) * 3 );
  hipMalloc( (void**)&d_b, sizeof( float ) * 3 );
  hipMalloc( (void**)&d_c, sizeof( float ) * 3 );

  // 1:1 with cudaMemcpy( d_a, &a, sizeof( float ) * 3, cudaMemcpyHostToDevice );
  hipMemcpy( d_a, &a, sizeof( float ) * 3, hipMemcpyHostToDevice );
  hipMemcpy( d_b, &b, sizeof( float ) * 3, hipMemcpyHostToDevice );
  hipMemcpy( d_c, &c, sizeof( float ) * 3, hipMemcpyHostToDevice );

  //VecAdd<<<1, N>>>( d_a, d_b, d_c );
  hipLaunchKernelGGL( VecAdd, dim3( 1 ), dim3( N ), 0, 0, d_a, d_b, d_c  );

  // cudaMemcpy( &c, d_c, sizeof( float ) * 3, cudaMemcpyDeviceToHost );
  hipMemcpy( &c, d_c, sizeof( float ) * 3, hipMemcpyDeviceToHost );

  printf( "before fence result C: %f %f %f\n", c[0], c[1], c[2] );

  // Fence all work
  //cudaDeviceSynchronize(); // can also call just to wait for 1 stream cudaStreamSynchronize(cudaStream)
  // printf( "after fence result C: %f %f %f\n", c[0], c[1], c[2] );

  /* Another example:
// Kernel invocation with one block of N * N * 1 threads
int numBlocks = 1;
dim3 threadsPerBlock(N, N);
MatAdd<<<numBlocks, threadsPerBlock>>>(A, B, C);
  */

  printf( "Normal exit\n" );
  return 0;
}
