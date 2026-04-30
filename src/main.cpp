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

#include "os/win32.h"
#include "os/win32.cpp"

#include "pencil/pencil.h"
#include "pencil/pencil.cpp"

void OutputDebugStringF(const char* fmt, ...)
{
  #if DEBUG_MODE
  va_list argptr;
  va_start(argptr, fmt);
  Scratch scratch = get_scratch(0, 0);
  Data_buffer buffer = data_buffer_make(scratch.arena, 128);
  int ret = vsprintf_s((char*)buffer.data, buffer.count, fmt, argptr);
  if (ret >= 0 && ret < buffer.count)
  {
    OutputDebugStringA((char*)buffer.data);
  } else { InvalidCodePath(); }
  end_scratch(&scratch);
  va_end(argptr);
  #endif
}

#define _CRT_SECURE_NO_WARNINGS
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"
#endif

void __debug_export_current_record_images(const Pencil_state* P, D3D_state* d3d)
{
  // todo: I dont like the api for the D3D_Texture_result. 
  //       The way we have to get the texture out and it is 0 if the succ is false
  //       and then when we have to release it and then the pointer is the result is kind of 
  //       also the same the one we released. Sucks. I would like there to just be a singe
  //       poiner or whatever that i have to work with.
  //       The less things to manage and think about, the better.

  // Loading up always_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result fresh_texture_res = d3d_texture_from_rtv(P->draw_texture_always_fresh);
    if (fresh_texture_res.succ)
    {
      Image_buffer image = d3d_export_texture(P->frame_arena, d3d, fresh_texture_res.texture);
      Assert(image.bytes_per_pixel == 4);
      stbi_flip_vertically_on_write(true); 
      int succ = stbi_write_png("always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      fresh_texture_res.texture->Release();
    } 
    else { Assert(0); }
  }

  /*
  // Loading up not_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result not_fresh_texture_res = d3d_texture_from_rtv(P->draw_texture_not_that_fresh);
    if (not_fresh_texture_res.succ)
    {
      Image_buffer image = d3d_export_texture(P->frame_arena, d3d, not_fresh_texture_res.texture);
      int succ = stbi_write_png("not_always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px);
      Assert(succ);
      not_fresh_texture_res.texture->Release();
    } 
    else { Assert(0); }
  }
  
  // Loading up current texture_after_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result texture_res = d3d_texture_from_rtv(P->current_record->texture_after_we_affected);
    if (texture_res.succ)
    {
      Image_buffer image = d3d_export_texture(P->frame_arena, d3d, texture_res.texture);
      int succ = stbi_write_png("current_texture_after_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px);
      Assert(succ);
      texture_res.texture->Release();
    } 
    else { Assert(0); }
  }

  // Loading up current texture_before_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result texture_res = d3d_texture_from_rtv(P->current_record->texture_before_we_affected);
    if (texture_res.succ)
    {
      Image_buffer image = d3d_export_texture(P->frame_arena, d3d, texture_res.texture);
      int succ = stbi_write_png("current_texture_before_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px);
      Assert(succ);
      texture_res.texture->Release();
    } 
    else { Assert(0); }
  }
  */
}

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
  win32_state->window_class.lpfnWndProc   = win32_proc;
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
  D3D_state d3d = {};
  {
    HRESULT hr = S_OK;
    #define HR(hr_value) HandleLater(hr == S_OK)

    // Factory
    {
      hr = CreateDXGIFactory2(0, IID_IDXGIFactory2, (void**)&d3d.dxgi_factory);
      HR(hr);
    }
    IDXGIFactory2* dxgi_factory = d3d.dxgi_factory;

    // Adapter
    {
      hr = dxgi_factory->EnumAdapters(0, &d3d.dxgi_adapter);
      HR(hr);
    }
    IDXGIAdapter* dxgi_adapter = d3d.dxgi_adapter;

    // Device, Context
    {
      D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };
      hr = D3D11CreateDevice(
        dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN /*D3D_DRIVER_TYPE_HARDWARE*/, Null, 
        D3D11_CREATE_DEVICE_DEBUG, levels, ArrayCount(levels),
        D3D11_SDK_VERSION, &d3d.device, Null, &d3d.context
      );
      HR(hr);
    }
    ID3D11Device* d3d_device = d3d.device;
    ID3D11DeviceContext* d3d_context = d3d.context;   

    // todo on release: Only use this for the debug version
    // Debug
    {
      // Debug for device
      ID3D11InfoQueue* debug_q = 0;
      hr = d3d.device->QueryInterface(IID_ID3D11InfoQueue, (void**)&debug_q);
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
      hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, win32_state->window_handle, &desc, Null, Null, &d3d.swap_chain);
      HR(hr);
    }
    IDXGISwapChain1* d3d_swap_chain = d3d.swap_chain;
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  Pencil_state P = {}; 

  P.arena = arena_alloc(Megabytes(64));
  P.frame_arena = arena_alloc(Megabytes(64));

  P.draw_texures_width = os_get_client_area_dims().x;
  P.draw_texures_height = os_get_client_area_dims().y;

  P.draw_texture_always_fresh = d3d_make_rtv(&d3d, P.draw_texures_width, P.draw_texures_height);
  P.draw_texture_not_that_fresh = d3d_make_rtv(&d3d, P.draw_texures_width, P.draw_texures_height);

  B32 rect_program_suc = true;
  P.rect_program = grafics_create_program_from_file(d3d.device, L"../data/rect_program_shader.hlsl", "vs_main", "ps_main", &rect_program_suc);
  Assert(rect_program_suc);

  B32 texture_to_screen_succ = true;
  P.texture_to_screen_program = grafics_create_program_from_file(d3d.device, L"../data/texture_to_screen_program_shader.hlsl", "vs_main", "ps_main", &texture_to_screen_succ);
  Assert(texture_to_screen_succ);

  F32 _color_[4] = { 0, 0, 1, 1 };
  d3d.context->ClearRenderTargetView(P.draw_texture_always_fresh, _color_);

  for (;!os_window_should_close();)
  {
    os_frame_begin();

    pencil_update(&P, false, &d3d);
    pencil_render(&P, &d3d);
    
    d3d.swap_chain->Present(0, 0);

    if (os_key_down(Key__Shift) && os_key_down(Key__Control) && os_key_went_up(Key__P)) {
      __debug_export_current_record_images(&P, &d3d);
      BP;
    }
    
  }

  // note: Not releasing stuff here since who cares, the os will release it for us

  return 0;
}

