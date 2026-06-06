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

// #include "pencil/pencil.h"
// #include "pencil/pencil.cpp"

void OutputDebugStringF(const char* fmt, ...);
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
    win32_state->window.window_class.style         = 0; // todo: Look into hredraw and vredraw
    win32_state->window.window_class.lpfnWndProc   = win32_proc;
    win32_state->window.window_class.hInstance     = app_instance;
    win32_state->window.window_class.hIcon         = Null;
    win32_state->window.window_class.hCursor       = Null;
    win32_state->window.window_class.hbrBackground = Null;
    win32_state->window.window_class.lpszMenuName  = Null;
    win32_state->window.window_class.lpszClassName = "pencil_app_flopper_class_name";
    win32_state->window.window_class.hIconSm       = Null;
  
    ATOM wc_atom = RegisterClassExA(&win32_state->window.window_class);
    Assert(wc_atom != 0);
    
    win32_state->window.is_transparent = true;
    win32_state->window.handle = CreateWindowExA(
      WS_EX_NOREDIRECTIONBITMAP,
      win32_state->window.window_class.lpszClassName,
      "Pencil",
      (MAIN_IS_DEBUGGING ? WS_OVERLAPPEDWINDOW : WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME)),
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

    BOOL succ = RegisterHotKey(win32_state->window.handle, 0, MOD_SHIFT|MOD_ALT, (MAIN_IS_DEBUGGING ? 'Q' : 'S'));
    HandleLater(succ);
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  // Pencil_state P = {}; 
  // P.current_mode = Pencil_mode__draw;
  // pencil_init(&P);
  
  // Testing and working on font provider
  FP_Font font = {};
  font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, range_u64_make(0, (U64)u8_max + 1));
  // {
  //   Scratch scratch = get_scratch(0, 0);
  //   Str8 path_to_fonts = os_get_path_to_system_fonts();
  //   Str8_list path_parts = {};
  //   str8_list_append_copy(scratch.arena, &path_parts, os_get_path_to_system_fonts());
  //   if (!str8_match(Str8FromC("\\"), str8_back(path_to_fonts, 1), Str8_match__normalise_slash)) { 
  //     str8_list_append_copy(scratch.arena, &path_parts, Str8FromC("\\"));
  //   }
  //   str8_list_append_copy(scratch.arena, &path_parts, Str8FromC("Arial.ttf"));
  //   Str8 path_to_font = str8_from_list(scratch.arena, &path_parts);
  //   font = fp_load_font(path_to_font, 128, range_u64_make(0, (U64)u8_max + 1));
  //   end_scratch(&scratch);
  // }

  // R_Target pen_texture = r_load_texture_from_file(Str8FromC("../data/pen.png"));
  R_Target window_frame_buffer_target = r_attach_window(win32_state->window);

  F64 prev_frame_duration_sec = 0.0;
  for (;!os_window_should_close();)
  {
    // if (P.terminate_app) { break; }

    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    // OutputDebugStringF("FPS: %f \n", 1.0/prev_frame_duration_sec);

    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    // if (!MAIN_IS_DEBUGGING)
    // if (hot_key_activated && !P.is_mid_drawing)
    // {
    //   hot_key_activated = false;
    //   os_window_set_mouse_passthrough(ToggleBool(os_window_is_mouse_passthrough()));
    // }

    { // UI and Application update 
      // pencil_do_ui(&P, font);
      // pencil_update(&P, false);
      // pencil_update(&P, !ui_has_active(), false);
    }

    { // Testing the ui for text input 
      d_fill_with_color(black());
      DeferLoop(ui_begin_build(os_get_window_dims(), os_get_mouse_pos()), ui_end_build())
      {
        ui_push_font(font);

        UI_PaddedBox(ui_px(100), Axis2__y)
        {
          ui_set_next_size_x(ui_px(200));
          ui_set_next_size_y(ui_px(100));
          ui_set_next_layout_axis(Axis2__x);
          ui_set_next_b_color(blue());
          UI_Box* wrapper = ui_box_make(Str8FromC("Wrapper id"), UI_Box_flag__has_background|UI_Box_flag__clip_x);
          
          wrapper->clip_data.clip_value[Axis2__x] = -50;

          // if (ui_actions_from_box(wrapper).is_down)
          // {
          //   BP;
          // }

          // todo: Understand how you do inputs and actions right now for the boxes
          // todo: Define how clip logic with inputs shoud work
          // todo: Implement the logic there. Also right now is a good time to just fix
          //       the hard looking code for actions to just have it be easily used later,
          //       not that we are doing ui again.

          UI_Parent(wrapper)
          {
            // todo: Have this element be clipped
            // [ ] - Childre that are outside are to not be visible
            // [ ] - Children that are outside should not be able to get inputs 
            // [ ] - Dont_draw flag should just clip on rendering side not the ligic ui side
            // [ ] - Clip shoud clip on the logic side for inputs and the render side 
            const U64 count = 4;
            V4F32 colors[count] = { red(), green(), orange(), magenta() };
            Str8 ids[count] = { Str8FromC("1"), Str8FromC("2"), Str8FromC("3"), Str8FromC("4") };
            for EachIndex(i, count)
            {
              ui_spacer(ui_px(15));              
              ui_set_next_size_x(ui_px(50));
              ui_set_next_size_y(ui_px(80));
              ui_set_next_b_color(colors[i]);
              UI_Box* box = ui_box_make(ids[i], UI_Box_flag__has_background);
            }

            if (ui_actions_from_id(ids[0]).is_hovered)
            {
              d_fill_with_color(green());
            }

            ui_spacer(ui_px(15));              
          }

          // { // Edit box
          //   static U8 buffer[64]    = {};
          //   static U64 buffer_count = 0;
          //   static U64 cursor_pos   = 0;
          //   static U64 section_pos  = 0;

          //   Scratch scratch = get_scratch(0, 0);
          //   {
          //     UI_Text_op_list op_list = ui_text_op_list_from_os_event_list(scratch.arena, os_get_frame_event_list());
          //     ui_aply_text_ops(op_list, buffer, ArrayCount(buffer), &buffer_count, &cursor_pos, &section_pos);
          //   }
          //   end_scratch(&scratch);

          //   ui_text_edit_box(v2f32(200, 100), buffer, buffer_count, cursor_pos, section_pos);
          // }
        }
      }
      
      // d_fill_with_color(black());
      // r_clear_target(window_frame_buffer_target, red());
      ui_draw();
    }

    { // Rendering
      // r_clear_target(window_frame_buffer_target, transparent());
      // pencil_render(&P);
      // if (!os_window_is_mouse_passthrough()) { ui_draw(); }      
    }

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, true);
    
    F64 frame_end_time_sec = os_get_time_for_timing_sec();
    prev_frame_duration_sec = frame_end_time_sec - frame_start_time_sec;

    #if DEBUG_MODE
    {
      // if (os_key_down(Key__shift) && os_key_down(Key__Control) && os_key_went_up(Key__P)) {
        // __debug_export_current_record_images(&P);
        // BP;
      // }
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

