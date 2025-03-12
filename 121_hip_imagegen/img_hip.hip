/**
 * Generate an image using the GPU, but through AMD ROCm HIP.
 *
 * 6 Mar 2025 by Anton Gerdelan.
 *
 * hipcc img_hip.hip
 */

#include <hip/hip_runtime.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// HIP Error-check macro
// See https://rocm.docs.amd.com/projects/HIP/en/docs-develop/how-to/hip_runtime_api/error_handling.html
#define HIP_CHECK( expression )                                                                                                                                \
  {                                                                                                                                                            \
    const hipError_t status = expression;                                                                                                                      \
    if ( status != hipSuccess ) { fprintf( stderr, "HIP error: %s at %s:%i\n", hipGetErrorString( status ), __FILE__, __LINE__ ); }                            \
  }

#define IMG_W 64
#define IMG_H 64
#define N_CHANS 3
#define N_PIXELS IMG_W* IMG_H

// ROCm data type support https://rocm.docs.amd.com/en/docs-6.0.2/about/compatibility/data-type-support.html
// int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t
// float8 (e4m3, e5m2) float16, bfloat16, tensorfloat32, float32, float64
__global__ void render_kernel( float* fb_ptr, int max_x, int max_y ) {
  int i = hipThreadIdx_x + hipBlockIdx_x * hipBlockDim_x;
  int j = hipThreadIdx_y + hipBlockIdx_y * hipBlockDim_y;
  if ( ( i >= max_x ) || ( j >= max_y ) ) return;

  int pixel_index         = j * max_x * 3 + i * 3;
  fb_ptr[pixel_index + 0] = (float)i / max_x;
  fb_ptr[pixel_index + 1] = (float)j / max_y;
  fb_ptr[pixel_index + 2] = 0.2;
}

int main() {
  hipDeviceProp_t devProp;
  HIP_CHECK( hipGetDeviceProperties( &devProp, 0 ) );
  printf( "ROCm HIP System: %i.%i %s\n", devProp.major, devProp.minor, devProp.name );

  printf( "Allocating unified memory for frame buffer..\n" );
  // Unified memory allows runtime to move framebuffer on demand to GPU for render and back to CPU for outputting image.
  // As opposed to HIP_CHECK( hipMalloc( (void**)&vram_img_ptr, img_sz ) );
  float* fb_ptr = NULL;
  size_t fb_sz  = N_CHANS * N_PIXELS * sizeof( float );
  HIP_CHECK( hipMallocManaged( (void**)&fb_ptr, fb_sz ) );

  {
    printf( "Launching kernel (rendering to frame buffer)..\n" );
    // https://rocm.docs.amd.com/projects/HIP/en/docs-5.7.0/reference/kernel_language.html
    int threads_x = 8, threads_y = 8;
    dim3 blocks  = dim3( IMG_W / threads_x + 1, IMG_H / threads_y + 1 );
    dim3 threads = dim3( threads_x, threads_y ); // Note CHANS is taken into account inside the kernel, not here.
    // kernel_name, dim3 griddim, dim3 blockdim, size_t dynamicshared, hipstream_t, kernel args...
    hipLaunchKernelGGL( render_kernel, blocks, threads, 0, 0, fb_ptr, IMG_W, IMG_H );
    // Check the kernel launch
    HIP_CHECK( hipGetLastError() );
    // Check for kernel execution error
    HIP_CHECK( hipDeviceSynchronize() );
  }

  // Not needed: HIP_CHECK( hipMemcpy( &img_ptr, vram_img_ptr, img_sz, hipMemcpyDeviceToHost ) );

  {
    printf( "Writing image...\n" );

    FILE* f_ptr = fopen( "output.ppm", "w" );
    fprintf( f_ptr, "P3\n%i %i\n255\n", IMG_W, IMG_H );
    for ( int j = IMG_H - 1; j >= 0; j-- ) {
      for ( int i = 0; i < IMG_W; i++ ) {
        size_t pixel_index = j * 3 * IMG_W + i * 3;
        float r            = fb_ptr[pixel_index + 0];
        float g            = fb_ptr[pixel_index + 1];
        float b            = fb_ptr[pixel_index + 2];
        int ir             = int( 255.99 * r );
        int ig             = int( 255.99 * g );
        int ib             = int( 255.99 * b );
        fprintf( f_ptr, "%i %i %i\n", ir, ig, ib );
      }
    }
    fclose( f_ptr );
  }

  // unsigned int res = apg_bmp_write( "output.bmp", img_ptr, IMG_W, IMG_H, N_CHANS ); <<----- crashing on access here
  // assert( res );

  // stbi_write_png( "output.png", IMG_W, IMG_H, N_CHANS, img_ptr, IMG_W * N_CHANS ); // <<----- crashing on access here

  printf( "Freeing memory..\n" );
  HIP_CHECK( hipFree( fb_ptr ) );

  printf( "Normal exit\n" );
  return 0;
}
