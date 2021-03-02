#include "d3d11_utils.hpp"
#define UNICODE
#define WIN32_LEAN_AND_MEAN // reduce size of windows header file
#include <windows.h>

#pragma comment( lib, "user32" )
#pragma comment( lib, "gdi32" )
#pragma comment( lib, "d3d11" )

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow ) {
  bool debug_device = true;
  d3d11_start( 640, 480, L"Anton's D3D11 Window", debug_device );
  bool should_quit = false;
  while ( !should_quit ) {
    should_quit = !d3d11_process_events();
    d3d11_present();
  }

  d3d11_stop();
  return 0;
}
