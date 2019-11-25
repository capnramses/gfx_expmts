/* Win32 starter code for Direct3D coding
 *  Based on Microsoft's https://docs.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program
 *  but modified to use the non-blocking PeekMessage() function
 *  Anton Gerdelan
 */

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#pragma comment( lib, "user32" ) // link against the win32 library
//#pragma comment( lib, "gdi32" )
//#pragma comment( lib, "d3d11" )
//#pragma comment( lib, "d3dcompiler.lib" )

// for getting some user input for camera controls etc
// can check for eg 'W' 'A' 'S' 'D' or VK_UP VK_DOWN VK_LEFT VK_RIGHT for arrow keys
bool keys_down[1024];

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow ) {
  OutputDebugString( L"Hello, World! -- debug printouts can use this function.." );

  // Register the window class.
  const wchar_t CLASS_NAME[] = L"Sample Window Class";

  WNDCLASS wc = {};

  wc.lpfnWndProc   = WindowProc;
  wc.hInstance     = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClass( &wc );

  // Create the window.

  HWND hwnd = CreateWindowEx( 0, // Optional window styles.
    CLASS_NAME,                  // Window class
    L"Learn to Program Windows", // Window text
    WS_OVERLAPPEDWINDOW,         // Window style

    // Size and position
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

    NULL,      // Parent window
    NULL,      // Menu
    hInstance, // Instance handle
    NULL       // Additional application data
  );

  if ( hwnd == NULL ) { return 0; }

  ShowWindow( hwnd, nCmdShow );

  MSG msg           = {};
  bool should_close = false;
  while ( !should_close ) {
    memset( keys_down, 0, sizeof( keys_down ) );

    if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
    if ( msg.message == WM_QUIT ) { break; }

    /* frame here */
  }

  return 0;
}

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch ( uMsg ) {
  case WM_KEYDOWN: {
    keys_down[wParam] = true;
  } break;
  case WM_DESTROY: PostQuitMessage( 0 ); return 0;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint( hwnd, &ps );

    FillRect( hdc, &ps.rcPaint, ( HBRUSH )( COLOR_WINDOW + 1 ) );

    EndPaint( hwnd, &ps );
  }
    return 0;
  }
  return DefWindowProc( hwnd, uMsg, wParam, lParam );
}