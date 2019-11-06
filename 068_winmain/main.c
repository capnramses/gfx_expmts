/* -- mostly adapted from Stefan&Francisco's BTH Slides
 * -- the tutorial example on Microsoft.com had bugs and was detected as a Trojan when compiled with cl.exe
 */

#ifndef UNICODE // NOTE(Anton) from Microsoft example
#define UNICODE
#endif

/* removes the need to add user32.lib and gdi32.lib to linking path */
#pragma comment( lib, "user32" )
#pragma comment( lib, "gdi32" )

#define WIN32_LEAN_AND_MEAN // reduce size of windows header file
#include <windows.h>

// callback for window events
LRESULT CALLBACK window_proc_cb( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch ( uMsg ) {
  case WM_DESTROY: {
    PostQuitMessage( 0 );
  } break;
  /* NOTE(Anton) this is just a placeholder to paint some stuff on the window so i remember how it works */
  case WM_PAINT: {
    HBRUSH pink_brush = CreateSolidBrush( 0x00FF00FF ); // pink (00bbggrr) - can also use RGB() macro to set channels
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint( hwnd, &ps );

    // All painting occurs here, between BeginPaint and EndPaint.
    // second param is the RECT to draw into - &ps.rcPaint is the entire window size
    RECT rc = { 32, 32, 256, 256 };
    FillRect( hdc, &rc, pink_brush );

    EndPaint( hwnd, &ps );
  } break;
  }
  return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow ) {
  HWND hwnd;
  { // Register a 'window class' by name then create the window from that class name.
    WNDCLASSEX wcex    = { 0 };
    wcex.cbSize        = sizeof( WNDCLASSEX );
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = window_proc_cb; // NOTE(Anton) fn ptr on next slide
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = L"BasicWindow";
    if ( !RegisterClassEx( &wcex ) ) { return 1; }
    RECT rc = { 0, 0, 640, 480 }; // resolution here
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

    hwnd = CreateWindow( L"BasicWindow", L"Anton Tries BasicWindow", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
      NULL, NULL, hInstance, NULL );
    if ( !hwnd ) { return 0; }
  }
  { // Show Window and stuff
    MSG msg = { 0 };
    ShowWindow( hwnd, nCmdShow );
    while ( WM_QUIT != msg.message ) {
      if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
      } else {
        // TODO(Anton) update / render here
      }
    }
    DestroyWindow( hwnd );
  }

  return 0;
}

/*
DirectX approach
1. Create window
2. Create Swap Chain, Device and Device Context (special DirectX Objects)
3. Create and establish the viewport
4. Load, Compile, Create shaders (vertex, pixel)
5. Geometry
  1. Define triangle data
  2. Create the vertex buffer
  3. Create the input layout
6. Render
  1. Clear window background
  2. Set/Unset each shader stage
  3. Set InputAssembly Vertex Buffer
  4. Set InputAssembly Topology
  5. Set InputAssembly Layout
  6. Draw geometry
  7. Switching the front and back buffer (SWAP)
*/
