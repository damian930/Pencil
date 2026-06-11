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

#include "render/render.h"
#include "render/render.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

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
    
    win32_state->window.is_transparent = true;
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

      // todo: Have this whole ui be under a drop down box that we then traverse as well
      static struct {
        B32 is_text_next_to_button;
        
        B32 is_check_box_checked;
        
        B32 is_radio_bool;
        U64 curent_radio_bool;
      } state = {};

      ui_set_next_flags(UI_Box_flag__has_background);
      ui_set_next_b_color(v4f32(0.25, 0.25, 0.25, 1.0));
      UI_Wrapper(Axis2__y)
      {
        // UI_PaddedBox(ui_px(25), Axis2__y)
        {
          UI_Row()
          {
            // Button
            ui_set_next_b_color(blue());
            UI_Actions button_actions = ui_button_f("Button##id");
            if (button_actions.is_clicked) { state.is_text_next_to_button = ToggleBool(state.is_text_next_to_button); }
            if (state.is_text_next_to_button) { 
              ui_spacer(ui_px(10));
              ui_label_f("__ Some text heer __"); 
            }
          }

          ui_spacer(ui_px(5));

          // Checkbox
          UI_Row()
          {
            ui_set_next_size_x(ui_px(25));
            ui_set_next_size_y(ui_px(25));
            ui_set_next_b_color(red());
            UI_Box* check_box = ui_box_make(Str8FromC("Check_box_button_id"), UI_Box_flag__has_background);
            UI_Actions check_box_actions = ui_actions_from_box(check_box);
            if (check_box_actions.went_down) {
              state.is_check_box_checked = ToggleBool(state.is_check_box_checked);
            }
            else if (check_box_actions.is_down) { 
              check_box->shape_style.vertex_colors[0].a = 0.25; 
              check_box->shape_style.vertex_colors[1].a = 0.25; 
              check_box->shape_style.vertex_colors[2].a = 0.25; 
              check_box->shape_style.vertex_colors[3].a = 0.25; 
            }
            else if (check_box_actions.is_hovered) { 
              check_box->shape_style.vertex_colors[0].a = 0.5; 
              check_box->shape_style.vertex_colors[1].a = 0.5; 
              check_box->shape_style.vertex_colors[2].a = 0.5; 
              check_box->shape_style.vertex_colors[3].a = 0.5; 
            }

            if (state.is_check_box_checked) {
              ui_set_b_color(check_box, green());
              ui_spacer(ui_px(10));
              ui_label_f("Checkbox checked");
            }
          }

          ui_spacer(ui_px(5));

          UI_Row()
          {
            // todo: when tab, just go to the next navigatable box

            for EachIndex(i, 3)
            {
              UI_Row()
              {
                ui_set_next_size_x(ui_px(15));
                ui_set_next_size_y(ui_px(15));
                ui_set_next_b_color((state.curent_radio_bool == i ? white() : blue()));
                UI_Box* radio = ui_box_make_f("radio_button##%lld", UI_Box_flag__has_background, i);
                UI_Actions radio_actions = ui_actions_from_box(radio);
                if (radio_actions.went_down) {
                  state.is_radio_bool = true;
                  state.curent_radio_bool = i; 
                }

                ui_spacer(ui_px(5));

                ui_label_f("radio_%lld", i);
              }

              ui_spacer(ui_px(10));
            }
          }
        }
      }

      Scratch scratch = get_scratch(0, 0);
      Str8 str = str8_copy_alloc(scratch.arena, ui_get_context()->navigated_box_id);
      OutputDebugStringF("Nav id: %s \n", str.data);
      end_scratch(&scratch);
    }

    r_clear_target(window_frame_buffer_target, black());
    ui_draw();

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, true);
  }

  return 0;
}
