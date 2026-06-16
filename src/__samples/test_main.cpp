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

  FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, rangeu64(0, (U64)u8_max + 1));
  R_Target window_frame_buffer_target = r_attach_window(win32_state->window);

  static V4F32 b_color_hsva = hsva_from_rgba(black());

  for (;!os_window_should_close();)
  {
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    DeferInitReleaseLoop(ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos()), ui_end_build())
    {
      ui_push_font(font);
      
      Str8 words[] = {
          Str8FromC("run"),
          Str8FromC("test"),
          Str8FromC("deploy"),
          Str8FromC("init"),
          Str8FromC("clean"),
          Str8FromC("install"),
          Str8FromC("update"),
          Str8FromC("sync"),
          Str8FromC("push"),
          Str8FromC("pull"),
          Str8FromC("fetch"),
          Str8FromC("status"),
          Str8FromC("log"),
          Str8FromC("diff"),
          Str8FromC("commit"),
          Str8FromC("checkout"),
          Str8FromC("merge"),
          Str8FromC("rebase"),
          Str8FromC("stash"),
      };

      static struct {
        U64 current_picked_command_index;
        B32 show_box = true;
      } ui_state = {};

      if (!ui_state.show_box)
      {
        for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
        {
          if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_down)
          {
            ui_state.show_box = true;
            os_consume_frame_event(ev);
            break;
          }
        }
      }

      if (ui_state.show_box)
      {
        UI_Row() UI_Padded(ui_p_of_p(1, 0))
        {
          UI_Col()
          {
            ui_spacer(ui_px(3));
  
            ui_size_x(ui_p_of_p(0.5, 1));
            ui_size_y(ui_p_of_p(0.5, 1));
            ui_b_color(blue());
            ui_border(1, red());
            ui_corner_r(0.15f);
            ui_layout_y();
            UI_Box* window_main_box = ui_box_make(Str8FromC("Window main box"), UI_Box_flag__has_background|UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners);
            UI_Parent(window_main_box) UI_Row() UI_Padded(ui_px(10)) UI_Col() UI_Padded(ui_px(10))
            {
              ui_size_x(ui_p_of_p(1, 0));
              ui_size_y(ui_fit());
              ui_border(3, magenta());
              ui_corner_r(0.15f);
              ui_flags(UI_Box_flag__has_rounded_corners|UI_Box_flag__has_borders);
              UI_Col() UI_Padded(ui_px(4)) UI_Row() UI_Padded(ui_px(4))
              {
                ui_label_f("Text here dude");
              }
  
              ui_spacer(ui_px(5));
  
              ui_size_x(ui_p_of_p(1, 0));
              ui_size_y(ui_p_of_p(1, 0));
              ui_layout_y();
              UI_Box* scroll_box_with_commandsui_box_make = ui_box_make(Str8FromC("Clip box with commands"), UI_Box_flag__clip);
              UI_Parent(scroll_box_with_commandsui_box_make)
              {
                for EachIndex(i, ArrayCount(words))
                {
                  // Command row box
                  ui_size_x(ui_p_of_p(1, 0));
                  ui_size_y(ui_fit());
                  ui_border(1, white());
                  ui_corner_r(0.5f);
                  ui_layout_x();
                  UI_Box* command_entry_box = ui_box_make_f("Comamnd entry box %lld ", UI_Box_flag__has_background|UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners, i);
                  UI_Parent(command_entry_box) UI_Padded(ui_p_of_p(1, 0))
                  {
                    ui_label(words[i]);
                  }
  
                  UI_Actions entry_acts = ui_actions_from_box(command_entry_box);
                  if (ui_state.current_picked_command_index == i) 
                  {
                    ui_set_b_color(command_entry_box, v4f32(1.0, 1.0, 1.0, 0.65));
                  }
                  else if (entry_acts.is_hovered)
                  {
                    ui_set_b_color(command_entry_box, v4f32(1.0, 1.0, 1.0, 0.4));
                  }

                  if (entry_acts.is_clicked)
                  {
                    ui_state.show_box = false;
                  }


                  ui_spacer(ui_px(5));
                }
  
                //todo: Scroll bar here for the scrolling of the things
              }
  
              UI_Actions scroll_box_acts  = ui_actions_from_box(scroll_box_with_commandsui_box_make);
              UI_Box_data scroll_box_data = ui_box_data_from_box_prev_frame(scroll_box_with_commandsui_box_make);
  
              if (ui_state.show_box)
              {
                for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
                {
                  if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__arrow_down && ev->key_event.went_down || ev->key_event.repeat_down)
                  {
                    ui_state.current_picked_command_index += 1;
                    if (ui_state.current_picked_command_index == ArrayCount(words)) {
                      ui_state.current_picked_command_index = 0;
                    }
                    os_consume_frame_event(ev);
                  }
                  if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__arrow_up && ev->key_event.went_down || ev->key_event.repeat_down)
                  {
                    os_consume_frame_event(ev);
                    if (ui_state.current_picked_command_index > 0) { ui_state.current_picked_command_index -= 1; }
                    else { ui_state.current_picked_command_index = ArrayCount(words) - 1; }
                  }
                }
              }

              F32 new_box_offset = scroll_box_data.clip_offset.y;
  
              // todo: Get the id for this thing here and if it is outside of the thing, then just map to it like for text edit coursor
              // Str8 id_for_picked_command_box = "Comamnd entry box %lld"
              // if (ui_state.current_picked_command_index)

              if (scroll_box_acts.is_hovered)
              {
                for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
                {
                  if (ev->kind == OS_Event_kind__wheel)
                  {
                    if (scroll_box_data.is_found)
                    {
                      new_box_offset += ev->wheel_event.scroll_data * 10; 
                      F32 max_offset = scroll_box_data.inner_content_dims.y - range_v2f32_dims(scroll_box_data.on_screen_bbox).y;
                      max_offset = Max(0.0f, max_offset);
                      clamp_f32_inplace(&new_box_offset, -max_offset, 0.0f);
                    }
  
                    os_consume_frame_event(ev);
                    break;
                  }
                }
              }
  
              ui_box_set_clip_offset_y(scroll_box_acts.new_box, new_box_offset);
            }
  
          }
        }
      }
    }
  
    r_clear_target(window_frame_buffer_target, rgba_from_hsva(b_color_hsva));
    ui_draw();

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, true);

  }
  return 0;
}
