/* Direct3D11 basic start reference code
 *  Anton Gerdelan
 */

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>       // D3D interface
#include <dxgi.h>        // graphics interface (adapters and swap chains)
#include <d3dcompiler.h> // shader compiler
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

  // vertex points. can replace this array with a 3D mesh loaded from a file.
  float vertex_data_array[] = {
    0.0f, 0.5f, 0.5f,   // point at top
    0.5f, -0.5f, 0.5f,  // point at bottom-right
    -0.5f, -0.5f, 0.5f, // point at bottom-left
  };

  HWND hwnd;
  UINT window_width = 800, window_height = 600;
  ID3D11Device* device_ptr                = NULL;
  ID3D11DeviceContext* device_context_ptr = NULL;          // D3D interface
  IDXGISwapChain* swap_chain_ptr;                          // back and front buffer swap interface
  ID3D11RenderTargetView* render_target_view_ptr   = NULL; // backbuffer interface
  ID3D11RasterizerState* rasterizer_state_ptr      = NULL;
  ID3D11DepthStencilView* depth_buffer_view_ptr    = NULL;
  ID3D11DepthStencilState* depth_stencil_state_ptr = NULL;

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
    swap_chain_descr.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;      // can also use DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
    swap_chain_descr.SampleDesc.Count                   = 1;                               // NOTE(Anton) various options here for multisampling
    swap_chain_descr.SampleDesc.Quality                 = 0;                               // NOTE(Anton) various options here for multisampling
    swap_chain_descr.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Use the surface or resource as an output render target.
    swap_chain_descr.BufferCount                        = 1;                               // NOTE(Anton) not sure if should be 1 or 2 here for double-buffering
    swap_chain_descr.OutputWindow                       = hwnd;                            // link output to our win32 window
    swap_chain_descr.Windowed                           = true; // MS reccs start window and allow user to FS via IDXGISwapChain::SetFullscreenState()
    swap_chain_descr.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD; // Discard the back buffer contents after presenting.
    HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &swap_chain_descr, &swap_chain_ptr,
      &device_ptr, &feature_level, &device_context_ptr );
    assert( S_OK == hr && swap_chain_ptr && device_ptr && device_context_ptr );

    /****** COLOUR BUFFER *******/
    ID3D11Texture2D* framebuffer;
    hr = swap_chain_ptr->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&framebuffer );
    assert( SUCCEEDED( hr ) );
    hr = device_ptr->CreateRenderTargetView( framebuffer, 0, &render_target_view_ptr );
    assert( SUCCEEDED( hr ) );
    framebuffer->Release();

    /****** DEPTH BUFFER AND DEPTH STENCIL *******/
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    framebuffer->GetDesc( &depthBufferDesc ); // base on framebuffer properties
    depthBufferDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* depthBuffer;
    hr = device_ptr->CreateTexture2D( &depthBufferDesc, nullptr, &depthBuffer );
    assert( SUCCEEDED( hr ) );
    hr = device_ptr->CreateDepthStencilView( depthBuffer, nullptr, &depth_buffer_view_ptr );
    assert( SUCCEEDED( hr ) );
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable              = TRUE;
    depthStencilDesc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc                = D3D11_COMPARISON_LESS;
    hr                                        = device_ptr->CreateDepthStencilState( &depthStencilDesc, &depth_stencil_state_ptr );
    assert( SUCCEEDED( hr ) );

    /***** set up winding convention (which triangle side is the front) and backface culling *****/
    D3D11_RASTERIZER_DESC rasteriser_descr = {};
    rasteriser_descr.FillMode              = D3D11_FILL_SOLID;
    rasteriser_descr.CullMode              = D3D11_CULL_BACK;
    // rasteriser_descr.FrontCounterClockwise = TRUE;
    hr = device_ptr->CreateRasterizerState( &rasteriser_descr, &rasterizer_state_ptr );
    assert( SUCCEEDED( hr ) );
  }
  ID3D11VertexShader* vertex_shader_ptr = NULL;
  ID3D11PixelShader* pixel_shader_ptr   = NULL;
  ID3D11InputLayout* input_layout_ptr   = NULL;
  { /***** Load shaders ****/
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG; // add more debug output
#endif
    ID3DBlob *vs_blob_ptr = NULL, *ps_blob_ptr = NULL, *error_blob = NULL;
    HRESULT hr = D3DCompileFromFile( L"shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs_main", "vs_5_0", flags, 0, &vs_blob_ptr, &error_blob );
    if ( FAILED( hr ) ) {
      if ( error_blob ) {
        OutputDebugStringA( (char*)error_blob->GetBufferPointer() );
        error_blob->Release();
      }
      if ( vs_blob_ptr ) { vs_blob_ptr->Release(); }
      assert( false );
    }
    hr = D3DCompileFromFile( L"shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps_main", "ps_5_0", flags, 0, &ps_blob_ptr, &error_blob );
    if ( FAILED( hr ) ) {
      if ( error_blob ) {
        OutputDebugStringA( (char*)error_blob->GetBufferPointer() );
        error_blob->Release();
      }
      if ( ps_blob_ptr ) { ps_blob_ptr->Release(); }
      assert( false );
    }
    hr = device_ptr->CreateVertexShader( vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), nullptr, &vertex_shader_ptr );
    assert( SUCCEEDED( hr ) );
    hr = device_ptr->CreatePixelShader( ps_blob_ptr->GetBufferPointer(), ps_blob_ptr->GetBufferSize(), nullptr, &pixel_shader_ptr );
    assert( SUCCEEDED( hr ) );
    /* map vertex variables to vertex shader input variables */
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
      { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      /*
      { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      */
    };
    hr = device_ptr->CreateInputLayout( inputElementDesc, ARRAYSIZE( inputElementDesc ), vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), &input_layout_ptr );
    assert( SUCCEEDED( hr ) );
  }

  ID3D11Buffer* vertex_buffer_ptr = NULL;
  UINT vertex_stride              = 3 * sizeof( float ); // 3 floats per vertex: float3 position
  UINT vertex_offset              = 0;
  UINT vertex_count               = 3; // 3 vertices in the mesh
  { /*** load mesh data into vertex buffer **/
    D3D11_BUFFER_DESC vertex_buff_descr     = {};
    vertex_buff_descr.ByteWidth             = sizeof( vertex_data_array );
    vertex_buff_descr.Usage                 = D3D11_USAGE_DEFAULT;
    vertex_buff_descr.BindFlags             = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sr_data          = { 0 };
    sr_data.pSysMem                         = vertex_data_array;
    HRESULT hr = device_ptr->CreateBuffer( &vertex_buff_descr, &sr_data, &vertex_buffer_ptr );
    assert( SUCCEEDED( hr ) );
  }

  MSG msg           = {};
  bool should_close = false;
  while ( !should_close ) {
    memset( keys_down, 0, sizeof( keys_down ) ); // reset memory of which keys are currently pressed

    /**** handle user input and other window events ****/
    if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
    if ( msg.message == WM_QUIT ) { break; }

    { /*** RENDER A FRAME ***/
      /* clear the back buffer to cornflower blue and clear the depth buffer for the new frame */
      float background_colour[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
      device_context_ptr->ClearRenderTargetView( render_target_view_ptr, background_colour );
      device_context_ptr->ClearDepthStencilView( depth_buffer_view_ptr, D3D11_CLEAR_DEPTH, 1.0f, 0 );

      /**** Rasteriser state and viewport *****/
      RECT winRect;
      GetClientRect( hwnd, &winRect );
      D3D11_VIEWPORT viewport = { 0.0f, 0.0f, ( FLOAT )( winRect.right - winRect.left ), ( FLOAT )( winRect.bottom - winRect.top ), 0.0f, 1.0f };
      device_context_ptr->RSSetViewports( 1, &viewport );
      device_context_ptr->RSSetState( rasterizer_state_ptr );

      /**** Output Merger *****/
      device_context_ptr->OMSetRenderTargets( 1, &render_target_view_ptr, nullptr );
      device_context_ptr->OMSetDepthStencilState( depth_stencil_state_ptr, 0 );

      /***** Input Assembler (map how the vertex shader inputs should be read from vertex buffer) ******/
      device_context_ptr->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
      device_context_ptr->IASetInputLayout( input_layout_ptr );
      device_context_ptr->IASetVertexBuffers( 0, 1, &vertex_buffer_ptr, &vertex_stride, &vertex_offset );
      /*** set vertex shader to use and pixel shader to use, and constant buffers for each ***/
      device_context_ptr->VSSetShader( vertex_shader_ptr, NULL, 0 );
      // device_context_ptr->VSSetConstantBuffers( 0, 1, &vs_constant_buffer );
      device_context_ptr->PSSetShader( pixel_shader_ptr, NULL, 0 );
      // device_context_ptr->PSSetConstantBuffers( 0, 1, &ps_constant_buffer );

      /*** draw the vertex buffer with the shaders ****/
      device_context_ptr->Draw( vertex_count, 0 );

      /**** swap the back and front buffers (show the frame we just drew) ****/
      swap_chain_ptr->Present( 1, 0 ); // sync interval, flags
    }                                  // end of frame
  }                                    // end of main loop

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