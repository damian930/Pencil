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

#include "dwmapi.h"
#include "dcomp.h"
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "dcomp.lib")

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

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
#include "__third_party/stb/stb_image_write.h"
#endif

void __debug_export_current_record_images(const Pencil_state* P, D3D_State* d3d)
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
      stbi_flip_vertically_on_write(true); 
      int succ = stbi_write_png("always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      fresh_texture_res.texture->Release();
    } 
    else { Assert(0); }
  }

  // Loading up not_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result not_fresh_texture_res = d3d_texture_from_rtv(P->draw_texture_not_that_fresh);
    if (not_fresh_texture_res.succ)
    {
      Image_buffer image = d3d_export_texture(P->frame_arena, d3d, not_fresh_texture_res.texture);
      int succ = stbi_write_png("not_always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
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
      int succ = stbi_write_png("current_texture_after_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
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
      int succ = stbi_write_png("current_texture_before_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      texture_res.texture->Release();
    } 
    else { Assert(0); }
  }
}

// todo: Move this up to have a separate part with globals for win32 main
B32 hot_key_activated = false;

LRESULT custom_win_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
  LRESULT result = {};
  if (message == WM_HOTKEY)
  {
    hot_key_activated = true;
    result = TRUE;
  } 
  else {
    result = CallWindowProc(win32_proc, window_handle, message, w_param, l_param);
  }
  return result;
}

void draw_ui(UI_Draw_command_list command_list, D3D_State* d3d, ID3D11RenderTargetView* rtv)
{
  for (UI_Draw_command* command = command_list.first; command; command = command->next)
  {
    d3d_draw_rect_pro(d3d, rtv, command->rect, command->rect_color, command->border_width, command->border_color);
  }
}

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
// int main()
{
  // HINSTANCE app_instance = GetModuleHandle(NULL);

  // Layers we allocated for the runtime 
  allocate_thread_context();
  os_init();
  ui_init();

  OS_State* win32_state = os_get_state();

  ///////////////////////////////////////////////////////////
  // - Window  
  //
  win32_state->window_class.cbSize        = sizeof(WNDCLASSEXA);
  win32_state->window_class.style         = 0; // todo: Look into hredraw and vredraw
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
    WS_EX_NOREDIRECTIONBITMAP,
    win32_state->window_class.lpszClassName,
    "Pencil",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    800, 600,
    Null,
    Null,
    app_instance, 
    Null
  );
  Assert(win32_state->window_handle != 0);

  // note: This doesnt seem to do anything, might remove it later 
  // Frame buffer transparency
  // {
  //   HRESULT hr       = {};
  //   BOOL composition = {}; 
  //   BOOL opaque      = {};
  //   DWORD color      = {};

  //   hr = DwmIsCompositionEnabled(&composition);
  //   HandleLater(hr == S_OK);
  //   HandleLater(composition);

  //   if (DwmGetColorizationColor(&color, &opaque) == S_OK && !opaque)
  //   {
  //     HRGN region = CreateRectRgn(0, 0, -1, -1);
  //     DWM_BLURBEHIND bb = {0};
  //     bb.dwFlags  = DWM_BB_ENABLE | DWM_BB_BLURREGION;
  //     bb.hRgnBlur = region;
  //     bb.fEnable  = TRUE;

  //     DwmEnableBlurBehindWindow(win32_state->window_handle, &bb);
  //     DeleteObject(region);
    // }
  // }

  // ShowWindow(win32_state->window_handle, SW_MAXIMIZE);
  ShowWindow(win32_state->window_handle, SW_SHOW);

  ///////////////////////////////////////////////////////////
  // - D3D 
  //
  D3D_State d3d = {};
  {
    HRESULT hr = S_OK;
    #define HR(hr_value) HandleLater(hr == S_OK)

    // Factory
    {
      hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_IDXGIFactory2, (void**)&d3d.dxgi_factory);
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
      hr = DXGIGetDebugInterface1(Null, IID_IDXGIInfoQueue, (void**)&dxgi_debug);
      HR(hr);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
      dxgi_debug->Release();
    }

    // Swap chain
    {
      V2F32 dims = os_get_client_area_dims();
      HandleLater(dims.x > 0.0f && dims.y > 0.0f);
      DXGI_SWAP_CHAIN_DESC1 desc = {};
      desc.Width       = (UINT)dims.x;
      desc.Height      = (UINT)dims.y;
      desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.Stereo      = FALSE;
      desc.SampleDesc  = { 1, 0 };
      desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      desc.BufferCount = 2;
      desc.Scaling     = DXGI_SCALING_STRETCH;//DXGI_SCALING_NONE; // todo: Learn what these do
      desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      desc.AlphaMode   = DXGI_ALPHA_MODE_PREMULTIPLIED;//DXGI_ALPHA_MODE_UNSPECIFIED;
      desc.Flags       = 0;
      hr = dxgi_factory->CreateSwapChainForComposition(d3d_device, &desc, Null, &d3d.swap_chain);
      // hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, win32_state->window_handle, &desc, Null, Null, &d3d.swap_chain);
      HR(hr);
    }
    IDXGISwapChain1* d3d_swap_chain = d3d.swap_chain;
  
    // Rect program
    {
      B32 rect_program_suc = true;
      d3d.rect_program = d3d_program_from_file(d3d.device, L"../data/rect_shader.hlsl", "vs_main", "ps_main", &rect_program_suc);
      HandleLater(rect_program_suc);
    }
  }

  ///////////////////////////////////////////////////////////
  // - Draw mode system wide hot key
  //
  {
    // Chaning the default winproc to be call withing a custom win proc for how keys
    // LONG_PTR set_succ_proc = SetWindowLongPtrA(win32_state->window_handle, GWLP_WNDPROC, (LONG_PTR)custom_win_proc);
    // Assert(set_succ_proc != 0);

    // BOOL succ = RegisterHotKey(win32_state->window_handle, 0, MOD_SHIFT|MOD_ALT, 'S');
    // HandleLater(succ);
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  Pencil_state P = {}; 
  {
    P.arena = arena_alloc(Megabytes(64));
    P.frame_arena = arena_alloc(Megabytes(64));
  
    P.draw_texures_width  = (U32)os_get_client_area_dims().x; // todo: Handle the case when the area is negative
    P.draw_texures_height = (U32)os_get_client_area_dims().y; // todo: Handle the case when the area is negative
  
    P.draw_texture_always_fresh = d3d_make_rtv(&d3d, P.draw_texures_width, P.draw_texures_height);
    P.draw_texture_not_that_fresh = d3d_make_rtv(&d3d, P.draw_texures_width, P.draw_texures_height);
  
    B32 texture_to_screen_succ = true;
    P.texture_to_screen_program = d3d_program_from_file(d3d.device, L"../data/texture_to_screen_program_shader.hlsl", "vs_main", "ps_main", &texture_to_screen_succ);
    Assert(texture_to_screen_succ);
  
    B32 circle_program_succ = true;
    P.circle_program = d3d_program_from_file(d3d.device, L"../data/circle_program_shader.hlsl", "vs_main", "ps_main", &circle_program_succ);
    Assert(circle_program_succ);
  }

  ID3D11BlendState* blendState;
  {
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    d3d.device->CreateBlendState(&desc, &blendState);
  }

  os_frame_begin();
  d3d_render_begin(&d3d, os_get_client_area_dims().x, os_get_client_area_dims().y);
  {
    ID3D11RenderTargetView* frame_buffer = d3d_get_frame_buffer_rtv(&d3d);
    F32 _color_[4] = { 1, 1, 1, 1 };
    d3d.context->ClearRenderTargetView(frame_buffer, _color_);
    frame_buffer->Release();
  }

  
  // todo: Do better with this here
  IDCompositionDevice* comp_device = 0;
  {
    HRESULT hr = {};
    
    IDXGIDevice* dxgi_device    = 0;
    IDCompositionVisual* visual = 0;
    IDCompositionTarget* target =  0;

    hr = d3d.device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
    HR(hr);
    
    hr = DCompositionCreateDevice(dxgi_device, __uuidof(comp_device), (void**)&comp_device);
    HR(hr);

    hr = comp_device->CreateVisual(&visual);
    HR(hr);
  
    hr = visual->SetContent((IUnknown*)d3d.swap_chain);
    HR(hr);
    
    hr = comp_device->CreateTargetForHwnd(win32_state->window_handle, true, &target);
    HR(hr);

    hr = target->SetRoot(visual);
    HR(hr);

    // dxgi_device->Release();
    // visual->Release();
    // target->Release();
  }

  for (;!os_window_should_close();)
  {
    os_frame_begin();
    d3d_render_begin(&d3d, os_get_client_area_dims().x, os_get_client_area_dims().y);

    pencil_update(&P, false, &d3d);
    pencil_render(&P, &d3d);

    /*
    // Testing ui for now
    // /*
    UI_Draw_command_list ui_draw_commands = {};
    {
      ui_begin_build(os_get_client_area_dims().x, os_get_client_area_dims().y, os_get_mouse_pos().x, os_get_mouse_pos().y);
      {
        UI_PaddedBox(ui_px(50), Axis2__x)
        {
          ui_set_next_size_x(ui_px(50));
          ui_set_next_size_y(ui_px(50));
          ui_set_next_b_color(red_f());
          ui_set_next_border(-10, yellow_f());
          UI_Box* box = ui_box_make(Str8{}, 0);
        }
     
      }
      ui_end_build();

      ui_draw_command_from_ui_root(P.frame_arena, ui_get_context()->root_box, &ui_draw_commands);
      ID3D11RenderTargetView* frame_buffer = d3d_get_frame_buffer_rtv(&d3d);
      draw_ui(ui_draw_commands, &d3d, frame_buffer);
      frame_buffer->Release();
    }
    */

    d3d_render_end(&d3d);
    os_frame_end();
    d3d.swap_chain->Present(1, 0);
    HRESULT commit_hr = comp_device->Commit();
    Assert(commit_hr == S_OK);

    #if DEBUG_MODE
    {
      if (os_key_down(Key__Shift) && os_key_down(Key__Control) && os_key_went_up(Key__P)) {
        __debug_export_current_record_images(&P, &d3d);
        BP;
      }
    }
    #endif
  }

  // note: Not releasing stuff here since who cares, the os will release it for us

  return 0;
}

