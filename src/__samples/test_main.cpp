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

      // Testing rounded corners and borders
      /*
      // UI_PaddedBox(ui_p_of_p(1, 0), Axis2__y)
      {
        static F32 smooth = 0.0f;

        ui_set_next_size_x(ui_px(200));
        ui_set_next_size_y(ui_px(100));
        ui_set_next_b_color(white());
        ui_set_next_corner_r(v4f32_all(0.25));
        ui_set_next_border(2, blue());
        ui_set_next_softness(smooth);
        ui_box_make(Str8{}, UI_Box_flag__has_background|UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners);
    
        ui_spacer(ui_px(50));

        UI_Row()
        {
          UI_BColor(blue())
          {
            if (ui_button(Str8FromC("UP")).is_clicked) { smooth += 1.0f; }
            if (ui_button(Str8FromC("DOWN")).is_clicked) { smooth -= 1.0f; }
          }
          clamp_f32_inplace(&smooth, 0.0f, 30.0f);
          ui_label_f("Smooth: %f", smooth);
        }
      }
      */

      /*
      UI_PaddedBox(ui_p_of_p(1, 0), Axis2__y)
      {
        ui_set_next_b_color(blue());
        UI_Actions button = ui_button_f("Button with hover");
        if (button.is_hovered) { 
          ui_set_b_color(button.new_box, change_alpha(blue(), 0.25)); 
          ui_set_cursor(OS_Cursor__hand);
        }

        ui_set_next_b_color(green());
        UI_Actions other_button = ui_button_f("Other button with hover");
        if (other_button.is_hovered) { 
          ui_set_b_color(other_button.new_box, change_alpha(green(), 0.25)); 
          ui_set_cursor(OS_Cursor__crosshair);
        }

        static U8 buffer[64]    = {};
        static U64 buffer_count = 0;
        static U64 cursor_pos   = 0;
        static U64 section_pos  = 0;

        static U64 index_of_selected_command = 0;

        V4F32 nice_grey = v4f32(0.15f, 0.17f, 0.20f, 1.0f);

        ui_set_next_flags(UI_Box_flag__has_background);
        ui_set_next_b_color(nice_grey);
        UI_PaddedBox(ui_px(3), Axis2__y);
        {
          ui_set_next_flags(UI_Box_flag__has_background);
          ui_set_next_b_color(pink());
          Str8 edit_box_id = Str8FromC("Text edit box");
          ui_text_edit_box(0.0f, edit_box_id, 200, buffer, &buffer_count, ArrayCount(buffer), &cursor_pos, &section_pos);
          if (ui_is_active_id(edit_box_id))
          {
            for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
            {
              if (ev->kind == OS_Event_kind__key && ev->key_event.went_down && ev->key_event.key == Key__arrow_up)
              {
                if (index_of_selected_command > 0) { index_of_selected_command -= 1; } 
                os_consume_frame_event(ev);
                break;
              }
              else if (ev->kind == OS_Event_kind__key && ev->key_event.went_down && ev->key_event.key == Key__arrow_down) {
                index_of_selected_command += 1;
                if (index_of_selected_command == ArrayCount(commands)) { index_of_selected_command -= 1; }
                os_consume_frame_event(ev);
                break;
              }
            }
          }
        }

        ui_spacer(ui_px(10));

        ui_set_next_flags(UI_Box_flag__has_background);
        ui_set_next_b_color(red());
        UI_Col()
        {
          for EachIndex(command_index, ArrayCount(commands))
          {
            Str8 command = commands[command_index];
            if (str8_is_front(command, str8_manual(buffer, buffer_count), 0)) {
              UI_Actions button_ac = ui_button(commands[command_index]);
              if (button_ac.is_navigated) { BP; }
              if (command_index == index_of_selected_command)
              {
                ui_set_b_color(button_ac.new_box, orange());
              }
              ui_spacer(ui_px(5));
            }
          }
        }
      }
      */

      ui_set_next_size_x(ui_children_sum());
      ui_set_next_size_y(ui_children_sum());
      ui_set_next_b_color(blue());
      ui_set_next_padding(5);
      UI_Parent(ui_box_make(Str8{}, UI_Box_flag__has_background))
      {
        // ui_set_next_size_x(ui_px(50));
        // ui_set_next_size_y(ui_px(50));
        // ui_set_next_b_color(red());
        // ui_box_make(Str8{}, UI_Box_flag__has_background);

        ui_set_next_flags(UI_Box_flag__has_background);
        ui_set_next_b_color(red());
        UI_Col()
        {
          ui_label_f("Label 1");
          ui_label_f("Label 2");
          ui_label_f("Label 3");
          ui_label_f("Label 4");
          ui_label_f("Label 5");
        }

        ui_spacer(ui_px(10));

        UI_Col()
        {
          ui_label_f("Label 1");
          ui_label_f("Label 2");
          ui_label_f("Label 3");
          ui_label_f("Label 4");
          ui_label_f("Label 5");
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
