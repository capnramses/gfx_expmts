/* Direct3D11 starter code
 *  Anton Gerdelan
 */

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <directxmath.h> // a maths library
#include <assert.h>
#include <stdio.h>
#include <string.h>

#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // directx library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface (driver layer)
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

// for getting some user input for camera controls etc
// can check for eg 'W' 'A' 'S' 'D' or VK_UP VK_DOWN VK_LEFT VK_RIGHT for arrow keys
bool keys_down[1024];

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow ) {
  OutputDebugString( L"Hello, World! -- debug printouts can use this function.." );

  HWND hwnd;
  UINT window_width = 800, window_height = 600;
  ID3D11DeviceContext* device_context_ptr = NULL;        // D3D interface
  IDXGISwapChain* swap_chain_ptr;                        // back and front buffer swap interface
  ID3D11RenderTargetView* render_target_view_ptr = NULL; // backbuffer interface

  { // *** CREATE WIN32 WINDOW ***
    const wchar_t CLASS_NAME[] = L"Anton's Window Class";
    WNDCLASS wc                = {};
    wc.lpfnWndProc             = WindowProc;
    wc.hInstance               = hInstance;
    wc.lpszClassName           = CLASS_NAME;
    RegisterClass( &wc );
    RECT rect = { 0, 0, (LONG)window_width, (LONG)window_height };
    AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
    hwnd = CreateWindow( CLASS_NAME, L"Anton's D3D Demo", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
      rect.bottom - rect.top, NULL, NULL, GetModuleHandle( NULL ), NULL );
    if ( hwnd == NULL ) { return 0; }
  }
  { // *** START DIRECT3D ***
    ID3D11Device* device_ptr;
    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3D11_CREATE_DEVICE_DEBUG; // adding this flag gives more helpful debug output
#endif
    DXGI_SWAP_CHAIN_DESC swap_chain_descr               = { 0 };
    swap_chain_descr.BufferDesc.RefreshRate.Numerator   = 0; // draw as fast as possible. can enable vsync here
    swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1; // draw as fast as possible. can enable vsync here
    swap_chain_descr.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_descr.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_descr.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;      // regular 32-bit colour output RGBA
    swap_chain_descr.SampleDesc.Count                   = 1;                               // NOTE(Anton) various options here for multisampling
    swap_chain_descr.SampleDesc.Quality                 = 0;                               // NOTE(Anton) various options here for multisampling
    swap_chain_descr.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Use the surface or resource as an output render target.
    swap_chain_descr.BufferCount                        = 1;                               // use 1 back buffer in addition to front buffer
    swap_chain_descr.OutputWindow                       = hwnd;                            // link output to our win32 window
    swap_chain_descr.Windowed                           = true; // MS reccs start window and allow user to FS via IDXGISwapChain::SetFullscreenState()
    swap_chain_descr.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD; // Discard the back buffer contents after presenting.
    HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &swap_chain_descr, &swap_chain_ptr,
      &device_ptr, &feature_level, &device_context_ptr );
    assert( S_OK == hr && swap_chain_ptr && device_ptr && device_context_ptr );
    ID3D11Texture2D* framebuffer;
    hr = swap_chain_ptr->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&framebuffer );
    assert( SUCCEEDED( hr ) );

    hr = device_ptr->CreateRenderTargetView( framebuffer, 0, &render_target_view_ptr );
    assert( SUCCEEDED( hr ) );
    framebuffer->Release();
  }

  MSG msg           = {};
  bool should_close = false;
  while ( !should_close ) {
    memset( keys_down, 0, sizeof( keys_down ) );

    if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
    if ( msg.message == WM_QUIT ) { break; }
    {   /*** RENDER A FRAME ***/
      { // clear the back buffer to cornflower blue
        float background_colour[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        device_context_ptr->ClearRenderTargetView( render_target_view_ptr, background_colour );
        RECT winRect;
        GetClientRect( hwnd, &winRect );
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, ( FLOAT )( winRect.right - winRect.left ), ( FLOAT )( winRect.bottom - winRect.top ), 0.0f, 1.0f };
        device_context_ptr->RSSetViewports( 1, &viewport );
        device_context_ptr->OMSetRenderTargets( 1, &render_target_view_ptr, nullptr );
      }

      // swap the back and front buffers
      swap_chain_ptr->Present( 1, 0 ); // sync interval, flags
    }
  }

  return 0;
}

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch ( uMsg ) {
  case WM_KEYDOWN: {
    keys_down[wParam] = true;
  } break;
  case WM_DESTROY: {
    PostQuitMessage( 0 );
    return 0;
  }
  default: break;
  }
  return DefWindowProc( hwnd, uMsg, wParam, lParam );
}