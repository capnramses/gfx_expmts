/** Reference:
 * https://github.com/ROCm/HIP-Examples/blob/master/HIP-Examples-Applications/HelloWorld/HelloWorld.cpp
 *
 * NOTES
 * seems 1:1 the same as CUDA, the dims things in CUDA << >> are just function arguments in HIP.
 * sudo apt install hipcc
 * hipcc needs a .cpp file - it gives weird errors with a .c.
 * printf() works inside a kernel.
 */

#include <hip/hip_runtime.h>
#include <stdio.h>

__global__ void hello_kernel( char* in, char* out ) {
  int num  = hipThreadIdx_x + hipBlockDim_x * hipBlockIdx_x;
  out[num] = in[num] + 1;
  printf("hello from kernel at hipThreadIdx_x=%zu hipBlockDim_x=%zu hipBlockIdx_x=%zu \n", hipThreadIdx_x,hipBlockDim_x,hipBlockIdx_x );
}

int main() {
  hipDeviceProp_t devProp;
  hipGetDeviceProperties( &devProp, 0 );
  printf( "System: %i.%i %s\n", devProp.major, devProp.minor, devProp.name );

  /* Initial input,output for the host and create memory objects for the kernel*/
  const char* input = "GdkknVnqkc";
  size_t strlength  = strlen( input );
  printf( "input string `%s`\n", input );
  char* output = (char*)malloc( strlength + 1 );

  char* inputBuffer;
  char* outputBuffer;
  hipMalloc( (void**)&inputBuffer, ( strlength + 1 ) * sizeof( char ) );
  hipMalloc( (void**)&outputBuffer, ( strlength + 1 ) * sizeof( char ) );

  hipMemcpy( inputBuffer, input, ( strlength + 1 ) * sizeof( char ), hipMemcpyHostToDevice );

  hipLaunchKernelGGL( hello_kernel, dim3( 1 ), dim3( strlength ), 0, 0, inputBuffer, outputBuffer );

  hipMemcpy( output, outputBuffer, ( strlength + 1 ) * sizeof( char ), hipMemcpyDeviceToHost );

  hipFree( inputBuffer );
  hipFree( outputBuffer );

  output[strlength] = '\0'; // Add the terminal character to the end of output.
  printf( "\noutput string: %s\n", output );

  free( output );

  return 0;
}
