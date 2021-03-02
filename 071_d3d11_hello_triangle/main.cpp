#include "d3d11_utils.hpp"
#define UNICODE
#define WIN32_LEAN_AND_MEAN // reduce size of windows header file
#include <windows.h>

#pragma comment( lib, "user32" )
#pragma comment( lib, "gdi32" )
#pragma comment( lib, "d3d11" )
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow ) {
  d3d11_start( 640, 480, L"Anton's D3D11 Window" );

  float triangle_buffer[] = {
    0.0f, 0.5f, 0.5f,   // point at top
    0.0f, 0.0f, 0.5f,   // blue assuming RGB
    0.5f, -0.5f, 0.5f,  // point at bottom-right
    0.5, 0.0f, 0.0f,    // red
    -0.5f, -0.5f, 0.5f, // point at bottom-left
    0.0f, 0.5f, 0.0f    // green
  };
  size_t triangle_buffer_sz = sizeof( triangle_buffer );
  Mesh triangle_mesh        = d3d11_create_mesh( triangle_buffer, (uint32_t)triangle_buffer_sz, 6 * sizeof( float ), 0, 3 );

  Shader shader = d3d11_create_shader_program( L"shader.hlsl", L"shader.hlsl" );

  bool should_quit = false;
  while ( !should_quit ) {
    should_quit = !d3d11_process_events();
    d3d11_clear_framebuffer( 102 / 256.0f, 2 / 256.0f, 60 / 256.0f, 1.0f ); // Imperial Purple

    /* draw stuff here */
    d3d11_draw_mesh( shader, triangle_mesh );

    d3d11_present();
  }

  d3d11_stop();
  return 0;
}
