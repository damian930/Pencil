/* Preface  
Its noon on 22nd of April 2025. Here I am sitting and about to start this thing called Pencil.
This was a project I decided to do for "Hand Made Essentials Jam". It was using raylib at first.
It would be better if I didnt use raylib and just used the platform or something that 
abstracts over the platform since I need some of the platform stuff that raylib doesnt have, so I
end up going around raylib a lot. 
I have never dont a project where I dont have an abstracted interface for all the platform stuff like Ryan.
But it is also hard to have that when you dont know the platforms. So here I am choosing to do the 
Casey/Martins style where the app connects to the platform and not platform to the app.
Will see if its bad on not. If it is, well i will learn about the platform more to then be able to
abstract. 
*/

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "platform.h"
#include "platform_win32.cpp"

#include "pencil/pencil.h"
#include "pencil/pencil.cpp"

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  allocate_thread_context();

  os_init();

  OS_State* win32_state = os_get_state();

  ///////////////////////////////////////////////////////////
  // - Window  
  //
  win32_state->window_class.cbSize        = sizeof(WNDCLASSEXA);
  win32_state->window_class.style         = 0;
  win32_state->window_class.lpfnWndProc   = win_proc;
  win32_state->window_class.hInstance     = app_instance;
  win32_state->window_class.hIcon         = Null;
  win32_state->window_class.hCursor       = Null;
  win32_state->window_class.hbrBackground = Null;
  win32_state->window_class.lpszMenuName  = Null;
  win32_state->window_class.lpszClassName = "d3d_1_window_class_name";
  win32_state->window_class.hIconSm       = Null;

  ATOM wc_atom = RegisterClassExA(&win32_state->window_class);
  Assert(wc_atom != 0);
  
  win32_state->window_handle = CreateWindowExA(
    0,
    win32_state->window_class.lpszClassName,
    "Pencil",
    WS_EX_LAYERED,
    CW_USEDEFAULT, CW_USEDEFAULT,
    800, 600,
    Null,
    Null,
    app_instance, 
    Null
  );
  Assert(win32_state->window_handle != 0);
  
  ShowWindow(win32_state->window_handle, SW_SHOW);

  ///////////////////////////////////////////////////////////
  // - D3D 
  //
  Grafics grafics = {};
  {
    HRESULT hr = S_OK;
    #define HR(hr_value) HandleLater(hr == S_OK)

    // Factory
    {
      hr = CreateDXGIFactory2(0, IID_IDXGIFactory2, (void**)&grafics.dxgi_factory);
      HR(hr);
    }
    IDXGIFactory2* dxgi_factory = grafics.dxgi_factory;

    // Adapter
    {
      hr = dxgi_factory->EnumAdapters(0, &grafics.dxgi_adapter);
      HR(hr);
    }
    IDXGIAdapter* dxgi_adapter = grafics.dxgi_adapter;

    // Device, Context
    {
      D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };
      hr = D3D11CreateDevice(
        dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN /*D3D_DRIVER_TYPE_HARDWARE*/, Null, 
        D3D11_CREATE_DEVICE_DEBUG, levels, ArrayCount(levels),
        D3D11_SDK_VERSION, &grafics.d3d_device, Null, &grafics.d3d_context
      );
      HR(hr);
    }
    ID3D11Device* d3d_device = grafics.d3d_device;
    ID3D11DeviceContext* d3d_context = grafics.d3d_context;   

    // todo on release: Only use this for the debug version
    // Debug
    {
      // Debug for device
      ID3D11InfoQueue* debug_q = 0;
      hr = grafics.d3d_device->QueryInterface(IID_ID3D11InfoQueue, (void**)&debug_q);
      HR(hr);

      debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
      debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
      debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
      debug_q->Release();

      // Debug for dxgi
      IDXGIInfoQueue* dxgi_debug = 0;
      hr = DXGIGetDebugInterface1(0, IID_IDXGIInfoQueue, (void**)&dxgi_debug);
      HR(hr);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
      dxgi_debug->Release();
    }

    // Swap chain
    {
      DXGI_SWAP_CHAIN_DESC1 desc = {};
      desc.Width       = 0;
      desc.Height      = 0;
      desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.Stereo      = FALSE;
      desc.SampleDesc  = { 1, 0 };
      desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      desc.BufferCount = 2;
      desc.Scaling     = DXGI_SCALING_NONE;
      desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
      desc.Flags       = 0;
      hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, win32_state->window_handle, &desc, Null, Null, &grafics.d3d_swap_chain);
      HR(hr);
    }
    IDXGISwapChain1* d3d_swap_chain = grafics.d3d_swap_chain;

    B32 rect_program_suc = true;
    grafics.rect_program = grafics_create_program_from_file(d3d_device, L"../data/rect_program_shader.hlsl", "vs_main", "ps_main", &rect_program_suc);
    Assert(rect_program_suc);

    B32 texture_to_screen_succ = true;
    grafics.texture_to_screen_program = grafics_create_program_from_file(d3d_device, L"../data/texture_to_screen_program_shader.hlsl", "vs_main", "ps_main", &texture_to_screen_succ);
    Assert(texture_to_screen_succ);

    /*
    ID3D11RasterizerState* rasterizer_state = 0;
    {
      // disable culling
      D3D11_RASTERIZER_DESC desc = {};
      desc.FillMode = D3D11_FILL_SOLID;
      desc.CullMode = D3D11_CULL_NONE;
      desc.DepthClipEnable = true;
      d3d_device->CreateRasterizerState(&desc, &rasterizer_state);
    }
    */

    // Creating fresh draw texture
    {
      D3D11_TEXTURE2D_DESC texture_desc = {};
      texture_desc.Width            = os_get_client_area_dims().x;
      texture_desc.Height           = os_get_client_area_dims().y;
      texture_desc.MipLevels        = 1;
      texture_desc.ArraySize        = 1;
      texture_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
      texture_desc.SampleDesc.Count = 1;
      texture_desc.Usage            = D3D11_USAGE_DEFAULT; // Read and write by the gpu
      texture_desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
      
      ID3D11Texture2D* texture = 0;
      hr = d3d_device->CreateTexture2D(&texture_desc, 0, &texture);
      HR(hr);

      hr = d3d_device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &grafics.draw_texture_rtv);
      HR(hr);

      texture->Release();
    }

    // Creating the rtv for the frame buffer
    {
      ID3D11Texture2D* backbuffer;
      d3d_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
      d3d_device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &grafics.frame_buffer_rtv);
      backbuffer->Release();
    }
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  for (;;)
  {
    arena_clear(win32_state->frame_events_arena);
    win32_state->frame_events = {};

    // Processing messages
    B32 stop_the_app = false;
    MSG msg;
    for (;PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
    {
      if (msg.message == WM_QUIT) { stop_the_app = true; break; }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    if (stop_the_app) { break; }

    pencil_update(&win32_state->frame_events, &grafics);
    // pencil_render();
    
    grafics.d3d_swap_chain->Present(0, 0);
  }

  // note: Not releasing stuff here since who cares, the os will release it for us

  return 0;
}

