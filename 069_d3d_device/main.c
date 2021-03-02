/* -- mostly adapted from Stefan&Francisco's BTH Slides
 * -- the tutorial example on Microsoft.com had bugs and was detected as a Trojan when compiled with cl.exe
 */

#ifndef UNICODE // NOTE(Anton) from Microsoft example
#define UNICODE
#endif

/* removes the need to add user32.lib and gdi32.lib to linking path */
#pragma comment( lib, "user32" )
#pragma comment( lib, "gdi32" )
#pragma comment( lib, "d3d11" )
//#pragma comment( lib, "dxgi" )
#pragma comment( lib, "dxguid" ) /* for IID_ID3D11Texture2D used in swap chain init -- TODO(Anton) find out why Kevin didn't need to use this */

#include "d3dutils.h"
#define COBJMACROS          /* this is for the C interface to D3D etc */
#define WIN32_LEAN_AND_MEAN // reduce size of windows header file
#include <windows.h>

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow ) {
  bool debug_device = true;
  int sample_count  = 1; /* does this have to be 1?? */
  d3d11_init( 640, 480, sample_count, L"Anton's D3D11 Window", debug_device );

  bool should_quit = false;
  while ( !should_quit ) {
    should_quit = !d3d11_process_events();
    d3d11_present();
  }

  d3d11_shutdown();

  return 0;
}

/*
DirectX approach
DONE 1. Create window
DONE 2. Create Swap Chain, Device and Device Context (special DirectX Objects)
NOT DONE 3. Create and establish the viewport
NOT DONE 4. Load, Compile, Create shaders (vertex, pixel)
NOT DONE 5. Geometry
  1. Define triangle data
  2. Create the vertex buffer
  3. Create the input layout
NOT DONE 6. Render
  1. Clear window background
  2. Set/Unset each shader stage
  3. Set InputAssembly Vertex Buffer
  4. Set InputAssembly Topology
  5. Set InputAssembly Layout
  6. Draw geometry
  7. Switching the front and back buffer (SWAP)
*/
