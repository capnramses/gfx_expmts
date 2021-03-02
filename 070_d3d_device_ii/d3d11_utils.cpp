#include "d3d11_utils.hpp"
#define UNICODE             // from Microsoft example
#define WIN32_LEAN_AND_MEAN // reduce size of Windows header file
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <assert.h>

static HWND _d3d11_window;
static IDXGISwapChain* _swap_chain_ptr;
static ID3D11Device* _device_ptr;
static ID3D11DeviceContext* _device_context_ptr;
static ID3D11RenderTargetView* _framebuffer_view_ptr;
static bool _quit_requested;

// NOTE(Anton) can get key presses in here.
LRESULT CALLBACK _window_proc_cb( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch ( uMsg ) {
  case WM_CLOSE: _quit_requested = true; return 0;
  case WM_DESTROY: {
    _quit_requested = true;
    PostQuitMessage( 0 );
  } break;
  case WM_ERASEBKGND: return TRUE;
  default: break;
  }
  return DefWindowProcW( hWnd, uMsg, wParam, lParam );
}

static void _d3d_create_device_and_swapchain( HWND hwnd, bool debug ) {
  D3D_FEATURE_LEVEL feature_level;
  UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
  if ( debug ) { flags |= D3D11_CREATE_DEVICE_DEBUG; }

  // You can also specify width, height, and display scaling methods under BufferDesc.
  DXGI_SWAP_CHAIN_DESC swap_chain_descr               = { 0 };
  swap_chain_descr.BufferDesc.RefreshRate.Numerator   = 60;                              // TODO(Anton) maybe I can remove this to use defaults?
  swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1;                               // TODO(Anton) maybe I can remove this to use defaults?
  swap_chain_descr.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;      // NOTE(Anton) seems to be the default
  swap_chain_descr.SampleDesc.Count                   = 1;                               // NOTE(Anton) various options here for MSAA
  swap_chain_descr.SampleDesc.Quality                 = 0;                               // NOTE(Anton) various options here for MSAA
  swap_chain_descr.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Use the surface or resource as an output render target.
  swap_chain_descr.BufferCount                        = 1; // TODO(Anton) i think this includes the back buffer and just counts front?
  swap_chain_descr.OutputWindow                       = hwnd;
  swap_chain_descr.Windowed                           = true; // MS reccs start window and allow user to FS via IDXGISwapChain::SetFullscreenState()

  HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &swap_chain_descr, &_swap_chain_ptr,
    &_device_ptr, &feature_level, &_device_context_ptr );
  assert( S_OK == hr && _swap_chain_ptr && _device_ptr && _device_context_ptr );
}

static HWND _create_win32_window( uint32_t w, uint32_t h, const wchar_t* window_title ) {
  HWND hwnd;
  // Register a 'window class' by name then create the window from that class name.
  WNDCLASSEX wcex    = { 0 };
  wcex.cbSize        = sizeof( WNDCLASSEX );
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)_window_proc_cb; // either &func or (WNDPROC)func
  wcex.hInstance     = GetModuleHandleW( NULL );
  wcex.lpszClassName = L"ANTOND3DWINDOWCLASS";
  if ( !RegisterClassEx( &wcex ) ) { MessageBoxA( 0, "RegisterClassEx failed", "Fatal Error", MB_OK ); }
  RECT rect = { 0, 0, (LONG)w, (LONG)h };
  AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );

  hwnd = CreateWindow( L"ANTOND3DWINDOWCLASS", window_title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
    rect.bottom - rect.top, NULL, NULL, GetModuleHandle( NULL ), NULL );

  return hwnd;
}

void d3d11_start( uint32_t w, uint32_t h, const wchar_t* window_title, bool debug ) {
  _d3d11_window = _create_win32_window( w, h, window_title );
  assert( _d3d11_window );
  _d3d_create_device_and_swapchain( _d3d11_window, debug );
  { // create a framebuffer render target
    ID3D11Texture2D* framebuffer;
    HRESULT hResult = _swap_chain_ptr->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&framebuffer );
    assert( SUCCEEDED( hResult ) );

    hResult = _device_ptr->CreateRenderTargetView( framebuffer, 0, &_framebuffer_view_ptr );
    assert( SUCCEEDED( hResult ) );
    framebuffer->Release();
  }
}

void d3d11_stop() {
  /* TODO(Anton) find cpp cleanup method for d3d11 stuff */
  DestroyWindow( _d3d11_window );
  _d3d11_window = 0;
  UnregisterClassW( L"ANTOND3DWINDOWCLASS", GetModuleHandleW( NULL ) );
}

bool d3d11_process_events() {
  MSG msg;
  while ( PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ) ) {
    if ( WM_QUIT == msg.message ) {
      _quit_requested = true;
    } else {
      TranslateMessage( &msg );
      DispatchMessageW( &msg );
    }
  }
  return !_quit_requested;
}

void d3d11_present() {
  FLOAT backgroundColor[4] = { 102/256.0f, 2/256.0f, 60/256.0f, 1.0f }; // Imperial Purple
  _device_context_ptr->ClearRenderTargetView( _framebuffer_view_ptr, backgroundColor );

  // NOTE(Anton) if you run the program in a loop w/o calling this line then it goes into infinite loop w/o displaying anything
  _swap_chain_ptr->Present( 1, 0 ); // sync interval, flags

  // NOTE(Anton) can handle window resizing here
}
