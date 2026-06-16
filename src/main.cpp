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

#define MAIN_IS_DEBUGGING true

#define _CRT_SECURE_NO_WARNINGS

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "__third_party/stb/stb_image_write.h"
#endif

void OutputDebugStringF(const char* fmt, ...);

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

#include "pencil/pencil.h"
#include "pencil/pencil.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

LRESULT custom_win_proc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

global B32 hot_key_activated = false;

#define APP_WINDOW_NAME      "Pencil"
#define APP_MUTEX_NAME_WIN32 "Pencil mutex that has a name that no one will ever know aobut. Last week was the kevin harts roast, shane did good."

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
    win32_state->window.window_class.style         = CS_HREDRAW|CS_VREDRAW/*| CS_DBLCLKS*/; // todo: Look into hredraw and vredraw
    win32_state->window.window_class.lpfnWndProc   = custom_win_proc;
    win32_state->window.window_class.hInstance     = app_instance;
    win32_state->window.window_class.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    win32_state->window.window_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    win32_state->window.window_class.hbrBackground = Null;
    win32_state->window.window_class.lpszMenuName  = Null;
    win32_state->window.window_class.lpszClassName = "pencil_app_flopper_class_name";
    win32_state->window.window_class.hIconSm       = Null;

    ATOM wc_atom = RegisterClassExA(&win32_state->window.window_class);
    Assert(wc_atom != 0);
    
    win32_state->window.is_transparent = false;
    win32_state->window.handle = CreateWindowExA(
      WS_EX_NOREDIRECTIONBITMAP,
      win32_state->window.window_class.lpszClassName,
      "Pencil",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      800, 600,
      Null,
      Null,
      app_instance, 
      Null
    );
    HandleLater(win32_state->window.handle != 0);
    
    ShowWindow(win32_state->window.handle, SW_MAXIMIZE);
 
    // Using WS_EX_TOOLWINDOW to not have the window shown in the task bar or be selected in the win+tab list of windows.
    // This has to be done after the maximizing. If maximizing is done first then the windows doest respect the task bar.
    // This allows the user to draw on the task bar, this is less convinient for the user.
    LONG ext_styles = GetWindowLong(win32_state->window.handle, GWL_EXSTYLE);
    SetWindowLong(win32_state->window.handle, GWL_EXSTYLE, ext_styles | WS_EX_TOOLWINDOW);
  }
  
  { // Making the window be on top of all other windows all the time
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
    // Chaning the default winproc to be called withing a custom win proc for hot keys
    LONG_PTR set_succ_proc = SetWindowLongPtrA(win32_state->window.handle, GWLP_WNDPROC, (LONG_PTR)custom_win_proc);
    Assert(set_succ_proc != 0);

    BOOL succ = RegisterHotKey(win32_state->window.handle, 0, MOD_SHIFT|MOD_ALT, (MAIN_IS_DEBUGGING ? 'Q' : 'S'));
    HandleLater(succ);
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  Pencil_state P = {}; 
  P.current_mode = Pencil_mode__draw;
  pencil_init(&P);
  
  FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, rangeu64(0, (U64)u8_max + 1));;

  R_Target window_frame_buffer_target = r_attach_window(win32_state->window);

  F64 frame_start_time_in_sec = 0.0;
  for (;!os_window_should_close();)
  {
    if (P.terminate_app) { break; }

    F64 prev_frame_start_time_in_sec = frame_start_time_in_sec;
    frame_start_time_in_sec = os_get_time_for_timing_sec();

    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__q)
      {
        return 0;
      }
    }

    { // UI and Application update 
      pencil_do_command_ui(&P, font);
      pencil_update(&P, ui_has_active());
    }

    { // Rendering
      r_clear_target(window_frame_buffer_target, transparent());
      pencil_render(&P);
      ui_draw();

      // todo: if cursor over the ui then set uis cursor
      //       else if ui is not then set to the pencils cursor

      U64 fps = (U64)(1.0 / (frame_start_time_in_sec - prev_frame_start_time_in_sec));
      #if 0
      d_draw_text_f("FPS: %lld", font, v2f32(0, 0), nice_green(), fps);
      #endif
    }

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, false);
  }

  // Not releasing anything since who cares, the system will release all the stuff

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
