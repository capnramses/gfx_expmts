#include "d3d11_utils.hpp"
#define UNICODE             // from Microsoft example
#define WIN32_LEAN_AND_MEAN // reduce size of Windows header file
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h> // shader compiler
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

static void _d3d_create_device_and_swapchain( HWND hwnd ) {
  D3D_FEATURE_LEVEL feature_level;
  UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined( DEBUG ) || defined( _DEBUG )
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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

// modified from https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--compile-a-shader
static HRESULT _d3d11_compile_shader( _In_ LPCWSTR src_filename, _In_ LPCSTR entry_point, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob ) {
  if ( !src_filename || !entry_point || !profile || !blob ) return E_INVALIDARG;
  *blob      = NULL;
  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
  flags |= D3DCOMPILE_DEBUG;
#endif
  const D3D_SHADER_MACRO defines[] = { "EXAMPLE_DEFINE", "1", NULL, NULL };
  ID3DBlob* shader_blob            = NULL;
  ID3DBlob* error_blob             = NULL;
  HRESULT hr = D3DCompileFromFile( src_filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point, profile, flags, 0, &shader_blob, &error_blob );
  if ( FAILED( hr ) ) {
    if ( error_blob ) {
      OutputDebugStringA( (char*)error_blob->GetBufferPointer() );
      error_blob->Release();
    }
    if ( shader_blob ) { shader_blob->Release(); }
    return hr;
  }
  *blob = shader_blob;
  return hr;
}

void d3d11_start( uint32_t w, uint32_t h, const wchar_t* window_title ) {
  _d3d11_window = _create_win32_window( w, h, window_title );
  assert( _d3d11_window );
  _d3d_create_device_and_swapchain( _d3d11_window );
  { // create a framebuffer render target
    ID3D11Texture2D* framebuffer;
    HRESULT hr = _swap_chain_ptr->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&framebuffer );
    assert( SUCCEEDED( hr ) );

    hr = _device_ptr->CreateRenderTargetView( framebuffer, 0, &_framebuffer_view_ptr );
    assert( SUCCEEDED( hr ) );
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

void d3d11_clear_framebuffer( float r, float g, float b, float a ) {
  FLOAT background_colour[4] = { r, g, b, a };
  _device_context_ptr->ClearRenderTargetView( _framebuffer_view_ptr, background_colour );

  RECT winRect;
  GetClientRect( _d3d11_window, &winRect );
  D3D11_VIEWPORT viewport = { 0.0f, 0.0f, ( FLOAT )( winRect.right - winRect.left ), ( FLOAT )( winRect.bottom - winRect.top ), 0.0f, 1.0f };
  _device_context_ptr->RSSetViewports( 1, &viewport );

  _device_context_ptr->OMSetRenderTargets( 1, &_framebuffer_view_ptr, nullptr );
}

void d3d11_present() {
  // NOTE(Anton) if you run the program in a loop w/o calling this line then it goes into infinite loop w/o displaying anything
  _swap_chain_ptr->Present( 1, 0 ); // sync interval, flags

  // NOTE(Anton) can handle window resizing here
}

// NOTE(Anton) it doesn't ask for the number or size of components, or the number of points
Mesh d3d11_create_mesh( const float* buffer, uint32_t buffer_sz, uint32_t vertex_stride, uint32_t vertex_offset, uint32_t n_vertices ) {
  Mesh mesh                = { 0 };
  ID3D11Buffer* buffer_ptr = NULL;

  D3D11_BUFFER_DESC buffer_descr = { 0 };
  buffer_descr.Usage             = D3D11_USAGE_DEFAULT;
  buffer_descr.ByteWidth         = buffer_sz;
  buffer_descr.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA sr_data = { 0 };
  sr_data.pSysMem                = buffer;
  sr_data.SysMemPitch            = 0;
  sr_data.SysMemSlicePitch       = 0;

  HRESULT hr = _device_ptr->CreateBuffer( &buffer_descr, &sr_data, &buffer_ptr );
  assert( SUCCEEDED( hr ) );

  mesh.vertex_buffer_ptr = (void*)buffer_ptr;
  mesh.n_vertices        = n_vertices;
  mesh.vertex_stride     = vertex_stride;
  mesh.vertex_offset     = vertex_offset;
  return mesh;
}

Shader d3d11_create_shader_program( const wchar_t* vs_filename, const wchar_t* ps_filename ) {
  assert( vs_filename && ps_filename );
  Shader shader                         = { 0 };
  ID3DBlob* vs_blob_ptr                 = NULL;
  ID3DBlob* ps_blob_ptr                 = NULL;
  ID3D11VertexShader* vertex_shader_ptr = NULL;
  ID3D11PixelShader* pixel_shader_ptr   = NULL;

  HRESULT hr = _d3d11_compile_shader( vs_filename, "VSMain", "vs_5_0", &vs_blob_ptr );
  assert( SUCCEEDED( hr ) );
  hr = _d3d11_compile_shader( ps_filename, "PSMain", "ps_5_0", &ps_blob_ptr );
  assert( SUCCEEDED( hr ) );

  hr = _device_ptr->CreateVertexShader( vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), NULL, &vertex_shader_ptr );
  assert( SUCCEEDED( hr ) );
  hr = _device_ptr->CreatePixelShader( ps_blob_ptr->GetBufferPointer(), ps_blob_ptr->GetBufferSize(), NULL, &pixel_shader_ptr );
  assert( SUCCEEDED( hr ) );

  ID3D11InputLayout* input_layout_ptr = NULL;
  {
    // NOTE(Anton) these names MUST match the designated names given in the vertex shader or it will assert here
    D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },                        //
      { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } //
    };
    hr = _device_ptr->CreateInputLayout( input_element_desc, ARRAYSIZE( input_element_desc ), vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), &input_layout_ptr );
    assert( SUCCEEDED( hr ) );
  }

  vs_blob_ptr->Release();
  ps_blob_ptr->Release();

  shader.vertex_shader_ptr = (void*)vertex_shader_ptr;
  shader.pixel_shader_ptr  = (void*)pixel_shader_ptr;
  shader.input_layout_ptr  = (void*)input_layout_ptr;

  return shader;
}

void d3d11_draw_mesh( Shader shader, Mesh mesh ) {
  _device_context_ptr->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
  _device_context_ptr->IASetInputLayout( (ID3D11InputLayout*)shader.input_layout_ptr );

  _device_context_ptr->VSSetShader( (ID3D11VertexShader*)shader.vertex_shader_ptr, NULL, 0 );
  _device_context_ptr->PSSetShader( (ID3D11PixelShader*)shader.pixel_shader_ptr, NULL, 0 );

  _device_context_ptr->IASetVertexBuffers( 0, 1, &(ID3D11Buffer*)mesh.vertex_buffer_ptr, &mesh.vertex_stride, &mesh.vertex_offset );

  _device_context_ptr->Draw( mesh.n_vertices, 0 );
}
