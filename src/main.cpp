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

#define _CRT_SECURE_NO_WARNINGS

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "__third_party/stb/stb_image_write.h"
#endif

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

#include "pencil/pencil.h"
#include "pencil/pencil.cpp"

void OutputDebugStringF(const char* fmt, ...);
LRESULT custom_win_proc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

global B32 hot_key_activated = false;

#define APP_WINDOW_NAME      "Pencil"
#define APP_MUTEX_NAME_WIN32 "Pencil mutex that has a name that no one will ever know aobut. Last week was the kevin harts roast, shane did good."

#define MAIN_IS_DEBUGGING true 

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  allocate_thread_context();
  os_init();
  r_init(); 
  d_init();
  fp_init();
  ui_init();

  { // Making sure that the app is not being run more than once
    HANDLE mutex_h = CreateMutexA(Null, false, APP_MUTEX_NAME_WIN32);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
      // Get the window of that application and return
      HWND window_h = FindWindowA(Null, APP_WINDOW_NAME);
      if (window_h)
      {
        SetForegroundWindow(window_h);
        if (IsIconic(window_h)) { // todo: What does this do ?
          ShowWindow(window_h, SW_RESTORE);
        }
        return 0;
      }
    }
  }

  OS_State* win32_state = os_get_state();

  ///////////////////////////////////////////////////////////
  // - Window  
  //
  {
    win32_state->window.window_class.cbSize        = sizeof(WNDCLASSEXA);
    win32_state->window.window_class.style         = 0; // todo: Look into hredraw and vredraw
    win32_state->window.window_class.lpfnWndProc   = win32_proc;
    win32_state->window.window_class.hInstance     = app_instance;
    win32_state->window.window_class.hIcon         = Null;
    win32_state->window.window_class.hCursor       = Null;
    win32_state->window.window_class.hbrBackground = Null;
    win32_state->window.window_class.lpszMenuName  = Null;
    win32_state->window.window_class.lpszClassName = "d3d_1_window_class_name";
    win32_state->window.window_class.hIconSm       = Null;
  
    ATOM wc_atom = RegisterClassExA(&win32_state->window.window_class);
    Assert(wc_atom != 0);
    
    win32_state->window.is_transparent = false;
    win32_state->window.handle = CreateWindowExA(
      WS_EX_NOREDIRECTIONBITMAP,
      win32_state->window.window_class.lpszClassName,
      "Pencil",
      WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME),
      // WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT,
      800, 600,
      Null,
      Null,
      app_instance, 
      Null
    );
    HandleLater(win32_state->window.handle != 0);
  
    if (MAIN_IS_DEBUGGING) {
      ShowWindow(win32_state->window.handle, SW_SHOW);
    } else {
      ShowWindow(win32_state->window.handle, SW_MAXIMIZE);
    }
    os_set_cursor(OS_Cursor__arrow);
  }
  
  { // Making the window be on top all the time
    BOOL succ = true;
    if (MAIN_IS_DEBUGGING) {
      succ = SetWindowPos(win32_state->window.handle, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    } else {
      succ = SetWindowPos(win32_state->window.handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
      os_window_set_mouse_passthrough(true);
    } 
    HandleLater(succ);
  }

  ///////////////////////////////////////////////////////////
  // - Draw mode system wide hot key
  //
  {
    // Chaning the default winproc to be call withing a custom win proc for how keys
    LONG_PTR set_succ_proc = SetWindowLongPtrA(win32_state->window.handle, GWLP_WNDPROC, (LONG_PTR)custom_win_proc);
    Assert(set_succ_proc != 0);

    BOOL succ = RegisterHotKey(win32_state->window.handle, 0, MOD_SHIFT|MOD_ALT, 'S');
    HandleLater(succ);
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  Pencil_state P = {}; 
  pencil_init(&P);

  // todo: Do better with this here
  //       Here is the link to the resource that explaince this code here and why it is needed
  //       https://learn.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine
  // todo: Move this into the renderer
  /*
  IDCompositionDevice* comp_device = 0;
  {
    HRESULT hr = {};
    
    // todo: Move this out of there
    D3D_State* d3d = r_get_state();

    IDXGIDevice* dxgi_device    = 0;
    IDCompositionVisual* visual = 0;
    IDCompositionTarget* target =  0;

    hr = d3d->device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
    HR(hr);
    
    hr = DCompositionCreateDevice(dxgi_device, __uuidof(comp_device), (void**)&comp_device);
    HR(hr);

    hr = comp_device->CreateVisual(&visual);
    HR(hr);
  
    hr = visual->SetContent((IUnknown*)d3d->swap_chain);
    HR(hr);
    
    hr = comp_device->CreateTargetForHwnd(win32_state->window.handle, true, &target);
    HR(hr);

    hr = target->SetRoot(visual);
    HR(hr);

    // dxgi_device->Release();
    // visual->Release();
    // target->Release();
  }
  */

  // Testing and working on font provider
  FP_Font font = {};
  {
    Scratch scratch = get_scratch(0, 0);
    Str8 path_to_fonts = os_get_path_to_system_fonts();
    Str8_list path_parts = {};
    str8_list_append_copy(scratch.arena, &path_parts, os_get_path_to_system_fonts());
    if (!str8_match(Str8FromC("\\"), str8_back(path_to_fonts, 1), Str8_match__normalise_slash)) { 
      str8_list_append_copy(scratch.arena, &path_parts, Str8FromC("\\"));
    }
    str8_list_append_copy(scratch.arena, &path_parts, Str8FromC("Arial.ttf"));
    Str8 path_to_font = str8_from_list(scratch.arena, &path_parts);
    font = fp_load_font(path_to_font, 18, range_u64_make(0, (U64)u8_max + 1));
    end_scratch(&scratch);
  }

  R_Chain swap_chain = r_attach_window(win32_state->window);
  R_Target swap_chain_target = r_target_from_swap_chain(swap_chain);

  F64 prev_frame_duration_sec = 0.0;
  for (;!os_window_should_close();)
  {
    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    OutputDebugStringF("FPS: %f \n", 1.0/prev_frame_duration_sec);

    os_frame_begin();
    r_render_begin(swap_chain);
    d_begin_batching(swap_chain_target);

    if (!MAIN_IS_DEBUGGING)
    if (hot_key_activated && !P.is_mid_drawing)
    {
      hot_key_activated = false;
      os_window_set_mouse_passthrough(ToggleBool(os_window_is_mouse_passthrough()));
    }
    
    { // UI and Application update 
      pencil_do_ui(&P, font);
      pencil_update(&P, !ui_has_active());
    }

    { // Rendering
      r_clear_chain(swap_chain, transparent());
      pencil_render(&P);
      if (!os_window_is_mouse_passthrough()) { ui_draw(); }
    }
    
    r_submit(swap_chain_target, d_get_batch_list());

    d_end_batching();
    r_render_end(swap_chain);
    os_frame_end();

    // Presenting 
    r_present_swap_chain(swap_chain, true);
    
    F64 frame_end_time_sec = os_get_time_for_timing_sec();
    prev_frame_duration_sec = frame_end_time_sec - frame_start_time_sec;

    #if DEBUG_MODE
    {
      if (os_key_down(Key__Shift) && os_key_down(Key__Control) && os_key_went_up(Key__P)) {
        // __debug_export_current_record_images(&P);
        BP;
      }
    }
    #endif
  }

  return 0;
}

///////////////////////////////////////////////////////////
// - Main helpers
//
// todo: Code for this is bad
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

LRESULT custom_win_proc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) 
{
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

