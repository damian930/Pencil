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

// UI_Box* next_box_for_box_depth_first__nullable(UI_Box* box)
// {
//   // box has to be not null 
//   // box has to have an id

//   if (ui_box_is_zero(box)) { return &__ui_g_zero_box; }
//   if (box->id.count == 0) { return &__ui_g_zero_box; }

//   // go down to the first child, if it is present then we have it
//   // if we dont have a child, then a sibling, if we dont have next sibling, then we move out to the 
//   // parent and have the parent go next since it was alredy used before
//   // Next sibling if present 

//   if (box->id.count == 0) { return 0; }
//   else if (!str8_match(original_id, box->id, 0)) { return box; }

//   for (UI_Box* child = box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//   {
//     next_box_for_box_depth_first__nullable(original_id, child);
//   }
//   return &__ui_g_zero_box;
// }

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

      // todo: Start traversing with keys
      static Str8 ids[]                = { Str8FromC("Button_1"), Str8FromC("Button_2"), Str8FromC("Button_3") };
      static B32 is_navigated_id_index = false;
      static U64 navigated_id_index    = 0;
      for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
      {
        if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__tab && ev->key_event.went_down)
        {
          is_navigated_id_index = true;
          navigated_id_index += 1;

          if (navigated_id_index >= ArrayCount(ids)) {
            navigated_id_index = 0;
          }

          // todo: This fucks up the ev->next if we consume the current event, then we dont have next 
          //       or maybe we do, but this is not great in terms of the expectancy for the code logic here,
          //       might need a better data structure here
          // os_consume_frame_event(ev);
        }
      }

      UI_PaddedBox(ui_px(100), Axis2__x)
      {
        ui_set_next_size_x(ui_children_sum());
        ui_set_next_size_y(ui_children_sum());
        ui_set_next_b_color(blue());
        UI_Box* wrapper = ui_box_make(Str8{}, UI_Box_flag__has_background);
        UI_Parent(wrapper)
        {
          UI_Col()
          {
            for EachIndex(i, ArrayCount(ids))
            {
              UI_PaddedBox(ui_px(5), Axis2__x)            
              {
                Scratch scratch    = get_scratch(0, 0);
                Str8 button_id     = ids[i];
                UI_Actions actions = ui_actions_from_id(button_id);

                ui_set_next_b_color(v4f32(0.25, 0.25, 0.25, 1.0));
                if (is_navigated_id_index && navigated_id_index == i) {
                  ui_set_active_id(button_id);
                  OutputDebugStringF("Button done via navigation \n");
                }

                if (actions.is_active) {
                  ui_set_next_b_color(orange());
                }
                else if (actions.is_down) {
                  is_navigated_id_index = false;
                  navigated_id_index = i;
                  ui_set_active_id(button_id);
                  OutputDebugStringF("Button done \n");
                }
                else if (actions.is_hovered) { ui_set_next_b_color(v4f32(0.75, 0.75, 0.75, 1.0)); }
                ui_button(button_id);

                ui_spacer(ui_px(5));
    
                end_scratch(&scratch);
              }
            }
          }
        }
      }

      ui_spacer(ui_px(25));

      ui_set_next_size_x(ui_px(100));
      ui_set_next_size_y(ui_px(100));
      ui_set_next_b_color(red());
      UI_Actions button = ui_button(Str8FromC("Button other"));
      if (button.is_down)
      {
        ui_set_active_id(button.new_box->id);
        ui_set_b_color(button.new_box, green());
        is_navigated_id_index = false;
      } 
      else {
        ui_reset_active_id(button.new_box->id);
      }
    }

    // Scratch scratch = get_scratch(0, 0);
    // Str8 hot_nt = str8_copy_alloc(scratch.arena, ui_get_context()->hot_box_id);
    // OutputDebugStringF("Hot id: %s \n", hot_nt.data == 0 ? "None" : (char*)hot_nt.data);
    // end_scratch(&scratch);

    r_clear_target(window_frame_buffer_target, black());
    ui_draw();

    r_submit(window_frame_buffer_target, d_get_batch_list());

    d_end_batching();
    os_frame_end();

    r_present(window_frame_buffer_target, true);
  }

  return 0;
}
