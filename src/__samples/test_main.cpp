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

#include "render/render.h"
#include "render/render.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

// #include "ui/widgets/ui_widgets.h"
// #include "ui/widgets/ui_widgets.cpp"

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

static Str8 commands[] = {
    Str8FromC("help"),
    Str8FromC("quit"),
    Str8FromC("list"),
    Str8FromC("clear"),
    Str8FromC("reload"),
    Str8FromC("status"),
    Str8FromC("connect"),
    Str8FromC("disconnect"),
    Str8FromC("run"),
    Str8FromC("stop"),
    Str8FromC("pause"),
    Str8FromC("resume"),
    Str8FromC("save"),
    Str8FromC("load"),
    Str8FromC("export"),
    Str8FromC("import"),
    Str8FromC("config"),
    Str8FromC("info"),
    Str8FromC("debug"),
    Str8FromC("version"),
};

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  allocate_thread_context();
  os_init();
  r_init(); 
  d_init();
  fp_init();
  ui_init();

  OS_State* win32_state = os_get_state();

  ///////////////////////////////////////////////////////////
  // - Window  
  //
  {
    win32_state->window.window_class.cbSize        = sizeof(WNDCLASSEXA);
    win32_state->window.window_class.style         = CS_DBLCLKS; // todo: Look into hredraw and vredraw
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
    
    win32_state->window.is_transparent = false;
    win32_state->window.handle = CreateWindowExA(
      WS_EX_NOREDIRECTIONBITMAP,
      win32_state->window.window_class.lpszClassName,
      "Test main window",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      800, 600,
      Null,
      Null,
      app_instance, 
      Null
    );
    ShowWindow(win32_state->window.handle, SW_SHOW);
  }
  
  os_set_cursor(OS_Cursor__arrow);

  FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, range_u64_make(0, (U64)u8_max + 1));
  R_Target window_frame_buffer_target = r_attach_window(win32_state->window);

  for (;!os_window_should_close();)
  {
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    DeferInitReleaseLoop(ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos()), ui_end_build())
    {
      ui_push_font(font);

      ui_set_next_size_x(ui_px(100));
      ui_set_next_size_y(ui_px(100));
      ui_set_next_b_color(blue());
      ui_set_next_child_gap(5);
      UI_Parent(ui_box_make(Str8{}, UI_Box_flag__has_background))
      {
        for EachIndex(i, 3)
        {
          ui_set_next_size_x(ui_px(12));
          ui_set_next_size_y(ui_px(12));
          ui_set_next_b_color(red());
          ui_set_next_padding(5);
          ui_box_make(Str8{}, UI_Box_flag__has_background);
        }
      }

      Scratch scratch = get_scratch(0, 0);
      Str8 str = str8_copy_alloc(scratch.arena, ui_get_context()->navigated_box_id);
      OutputDebugStringF("Nav id: %s \n", str.data);
      end_scratch(&scratch);
    }

    r_clear_target(window_frame_buffer_target, black());
    // r_clear_target(window_frame_buffer_target, red());
    ui_draw();

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, true);
  }

  return 0;
}
