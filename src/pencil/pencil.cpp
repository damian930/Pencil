#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "pencil.h"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

Draw_record* get_new_draw_record_from_pool__nullable(Pencil_state* P)
{
  Draw_record* result = 0;
  if (P->first_free_draw_record)
  {
    result = P->first_free_draw_record;
    DllPopFront_Name(P, first_free_draw_record, last_free_draw_record, next, prev);
    *result = Draw_record{};
  }
  else if (P->count_of_pool_draw_records_in_use < DRAW_RECORDS_MAX_COUNT)
  {
    result = P->pool_of_draw_records + P->count_of_pool_draw_records_in_use;
    P->count_of_pool_draw_records_in_use += 1;
    *result = Draw_record{};
  }
  return result;
}

// todo: Does this really need to care about is_ui_capturing_mouse, or shoud this
//       be handled by the caller ????
Draw_record_registration_result register_new_draw_record(Pencil_state* P, B32 is_ui_capturing_mouse)
{
  if (P->is_mid_drawing || is_ui_capturing_mouse) { return Draw_record_registration_result{}; }

  // Freeing all the records that are in front of the current one
  if (P->current_record != 0)
  {
    for (Draw_record* record = P->last_record; record != 0;) 
    {
      if (record == P->current_record) { break; }
      record->texture_before_we_affected->Release();
      record->texture_after_we_affected->Release();
      
      DllPopBack_Name(P, first_record, last_record, next, prev);
      Draw_record* prev_record = record->prev;
      
      *record = Draw_record{};
      DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
      
      record = prev_record;
    }
  }

  Draw_record* new_draw_record = get_new_draw_record_from_pool__nullable(P);
  if (new_draw_record == 0)
  {
    Draw_record* oldest_record = P->first_record;
    DllPopFront_Name(P, first_record, last_record, next, prev);
    
    *oldest_record = Draw_record{};
    DllPushBack_Name(P, oldest_record, first_free_draw_record, last_free_draw_record, next, prev);

    new_draw_record = get_new_draw_record_from_pool__nullable(P);
  }
  Assert(new_draw_record != 0); // This has to be true, its an ivariant

  // Adding the new draw record to the draw record queue
  DllPushBack_Name(P, new_draw_record, first_record, last_record, next, prev);  Assert(P->last_record == new_draw_record);

  new_draw_record->texture_before_we_affected = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  HandleLater(new_draw_record->texture_before_we_affected != 0);

  new_draw_record->texture_after_we_affected = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  HandleLater(new_draw_record->texture_after_we_affected != 0);

  P->current_record = new_draw_record;

  Draw_record_registration_result result = {};
  result.succ = true;
  result.record = new_draw_record;

  return result;
}
 
void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse)
{
  // Handling signals
  {
    if (P->signal_new_pen_size)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->pen_size = P->new_pen_size;
        P->signal_new_pen_size = false;
        P->new_pen_size = 0;
      }
    }
    else
    if (P->signal_new_eraser_size)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->eraser_size = P->new_eraser_size;
        P->signal_new_eraser_size = false;
        P->new_eraser_size = 0;
      }
    }
    else 
    if (P->signal_swap_to_eraser)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->signal_swap_to_eraser = false;
        P->is_erasing_mode = true;
      }
    }
    else 
    if (P->signal_swap_to_pen)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->signal_swap_to_pen = false;
        P->is_erasing_mode = false;
      }
    }
  }
  
  B32 dont_start_drawing_this_frame = false;

  // These are different modes that we might want to change
  // if (!P->is_mid_drawing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)) 
  if (!P->is_mid_drawing && os_key_down(Key__Control) && os_key_down(Key__Shift) && os_key_went_down(Key__Z)) 
  {
    dont_start_drawing_this_frame = true; 

    // todo: Look into this case, might be not implemented well 
    // This is here to be able to deal with current_record beeing 0
    Draw_record* next_record = 0;
    if (P->current_record == 0) {
      next_record = P->first_record;
    } 
    else if (P->current_record->next != 0) {
      next_record = P->current_record->next;
    }

    if (next_record)
    {
      ID3D11RenderTargetView* future_texture = next_record->texture_after_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      r_copy_from_texture_to_texture(P->draw_texture_always_fresh, future_texture);

      // Change the affected part from the not so fresh texture to the stored old version
      r_copy_from_texture_to_texture(P->draw_texture_not_that_fresh, future_texture);

      P->current_record = next_record;
    }
  }
  else // User wants to remove the last line they drew
  if (!P->is_mid_drawing && os_key_down(Key__Control) && os_key_went_down(Key__Z)) 
  {
    dont_start_drawing_this_frame = true;

    if (P->current_record != 0)
    {
      Draw_record* record = P->current_record;
      ID3D11RenderTargetView* texture_before_we_affected = record->texture_before_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      r_copy_from_texture_to_texture(P->draw_texture_always_fresh, texture_before_we_affected);

      // Change the affected part from the not so fresh texture to the stored old version
      r_copy_from_texture_to_texture(P->draw_texture_not_that_fresh, texture_before_we_affected);

      P->current_record = P->current_record->prev;
    }
  }
  else // User want to clear the screen
  if (!P->is_mid_drawing && os_key_went_up(Key__Delete))
  {
    dont_start_drawing_this_frame = true;

    // Creating a new current record
    Draw_record_registration_result record_reg = register_new_draw_record(P, is_ui_capturing_mouse);
    if (record_reg.succ)
    {
      Draw_record* record = record_reg.record;
      P->current_record = record_reg.record;
    
      U64 w = P->draw_texures_width;
      U64 h = P->draw_texures_height;
      
      // Clearing the texture
      r_clear_rtv(P->draw_texture_always_fresh, transparent());
  
      // Storing the prev texture state
      r_copy_from_texture_to_texture(P->current_record->texture_before_we_affected, P->draw_texture_not_that_fresh);
  
      // Storing the new texture state
      r_copy_from_texture_to_texture(P->current_record->texture_after_we_affected, P->draw_texture_always_fresh);

      // Matching the prev texture state to the new one
      r_copy_from_texture_to_texture(P->draw_texture_not_that_fresh, P->draw_texture_always_fresh);
    }
  }
  else // User wants to start using the eraser pen 
  if (!P->is_mid_drawing && os_key_went_up(Key__E))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = true;
  }
  else // User wants to start using the brush/pen
  if (!P->is_mid_drawing && os_key_went_up(Key__B))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = false;
  } 
  else
  if (os_key_went_down(Key__Tab))
  {
    P->show_brush_ui_menu = ToggleBool(P->show_brush_ui_menu);
    ui_reset_active_id_match(P->brush_menu_ui_id);
  }

  if (dont_start_drawing_this_frame) { goto __active_draw_update_routine_end__; }

  if (!P->is_mid_drawing && !is_ui_capturing_mouse && os_mouse_button_down(Mouse_button__Left)) 
  {
    Draw_record_registration_result record_registation = register_new_draw_record(P, is_ui_capturing_mouse);
    if (record_registation.succ)
    {
      // todo: What is .succ here about, do we need it ???
      P->is_mid_drawing = true;
    }
  }
  else // Updating active drawing 
  if (P->is_mid_drawing && os_mouse_button_down(Mouse_button__Left))
  {
    V2F32 new_pos = os_get_mouse_pos();
    V2F32 prev_pos = os_get_prev_mouse_pos();
    
    // Drawing a continuos line based on delta
    {
      V4F32 color  = P->pen_color;
      F32 pen_size = (F32)P->pen_size;
      if (P->is_erasing_mode) { 
        color    = v4f32(0.0f, 0.0f, 0.0f, 0.0f); 
        pen_size = (F32)P->eraser_size;
      }

      F32 dx = new_pos.x - prev_pos.x;
      F32 dy = new_pos.y - prev_pos.y;
      F32 length = sqrtf(dx * dx + dy * dy);
      U64 steps = (U64)length;
      for (U64 i = 0; i <= steps; i++) 
      {
        F32 t = (steps == 0) ? 0.0f : (F32)i / steps;
        F32 x = prev_pos.x + dx * t;
        F32 y = prev_pos.y + dy * t;
        r_draw_circle(P->draw_texture_always_fresh, x, y, pen_size, color, P->is_erasing_mode);
      }
    }

    // todo: Update this comment here (We no longer use cpu side image)
    // note: Here is the only time when the cpu side image is not synced to the gpu one. 
    //       But we do later sync it when we stop drawing.
  }
  else // Here we finalise the draw recordd that the user have been drawing
  if (P->is_mid_drawing && os_mouse_button_went_up(Mouse_button__Left))
  {
    Assert(P->current_record != 0);
    Assert(P->current_record->texture_after_we_affected != 0);  // These are expected to already be allocated by this point
    Assert(P->current_record->texture_before_we_affected != 0); // These are expected to already be allocated by this point
    
    P->is_mid_drawing = false;

    U64 draw_t_width = P->draw_texures_width;
    U64 draw_t_height = P->draw_texures_height;

    Draw_record* record = P->current_record;
    ID3D11RenderTargetView* texture_before_we_affected = record->texture_before_we_affected;
    ID3D11RenderTargetView* texture_after_we_affected  = record->texture_after_we_affected;
    
    // note: By this point, the fresh draw texture is the new final version of what the user has draw

    // Storing the prev version of the draw texture 
    r_copy_from_texture_to_texture(texture_before_we_affected, P->draw_texture_not_that_fresh);

    // Storing the new version of the draw texture
    r_copy_from_texture_to_texture(texture_after_we_affected, P->draw_texture_always_fresh);

    // Updating the prev version of the draw texture to match the new version
    r_copy_from_texture_to_texture(P->draw_texture_not_that_fresh, P->draw_texture_always_fresh);
  }

  __active_draw_update_routine_end__: {};
}

// todo: I would like to pass P here as const, and signals as a separate thing then to have it clear that ui doesnt modify the state at all
void pencil_do_ui(Pencil_state* P, FP_Font font)
{ 
  ProfileFuncBegin();

  ui_begin_build(os_get_client_area_dims().x, os_get_client_area_dims().y, os_get_mouse_pos().x, os_get_mouse_pos().y);

  ui_push_font(font);

  ui_set_next_border(2, v4f32(1, 0, 0, 1));
  ui_set_next_size_x(ui_p_of_p(1, 1));
  ui_set_next_size_y(ui_p_of_p(1, 1));
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* top_wrapper = ui_box_make(Str8{}, 0);

  UI_Box* content_inner = 0;
  UI_Parent(top_wrapper)
  {
    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_b_color(v4f32(1, 0, 0, 1));
    UI_Box* left_border = ui_box_make(Str8{}, 0);
    
    ui_set_next_size_x(ui_p_of_p(1, 0));
    ui_set_next_size_y(ui_p_of_p(1, 0));
    ui_set_next_layout_axis(Axis2__y);
    UI_Box* content_outer = ui_box_make(Str8{}, 0);

    UI_Parent(content_outer)
    {
      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_b_color(red());
      UI_Box* top_border = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 0));
      ui_set_next_size_y(ui_p_of_p(1, 0));
      ui_set_next_layout_axis(Axis2__y);
      content_inner = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_b_color(red());
      UI_Box* bottom_border = ui_box_make(Str8{}, 0);
    }

    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_b_color(v4f32(1, 0, 0, 1));
    UI_Box* right_border = ui_box_make(Str8{}, 0);
  }

  // todo: Make this not limited, bur rather grow
  UI_PaddedBox(ui_px(100), Axis2__y)
  UI_Parent(content_inner)
  {
    ui_set_next_size_x(ui_children_sum());
    ui_set_next_size_y(ui_children_sum());
    ui_set_next_b_color(black());
    UI_Box* brush_box = ui_box_make(Str8FromC("Brush box menu id"), 0);
    
    UI_Parent(brush_box)
    {
      ui_set_next_size_x(ui_px(500));
      ui_set_next_size_y(ui_px(50));
      ui_set_next_layout_axis(Axis2__x);
      ui_set_next_b_color(blue());
      UI_Box* menu_name_box = ui_box_make(Str8FromC("Brush box menu name box id"), 0);

      UI_Parent(menu_name_box)
      {
        // notes:
        // Here i need to center the label
        // I could i either make a flag in the ui core that would center the text box in the end on y or x.
        // I could just wrap it up all the time, this would not be theat great, since we would not be able 
        // to handle cases when the parent is children sizes, for that i could introduce a new size
        // that would limit the p_of_p to not expend pass the immediate parent, so it would be like
        // clayes grow in a way

        ui_spacer(ui_px(10));
        ui_label(Str8FromC("Brushes"));

        // ui_spacer(ui_p_of_p(1, 0));
        // ui_spacer(ui_p_of_p(1, 0));
      }

      ui_spacer(ui_px(50));

      UI_Row()
      {
        static V4F32 hsv = hsv_from_rgb(blue());

        F32 new_hsv = 0.0f;
        ui_color_picker_h(Str8FromC("Hue picker"), ui_px(150), ui_px(350), Axis2__x, hsv.hue, &new_hsv);
        hsv.hue = new_hsv;

        ui_spacer(ui_px(25));

        F32 new_sat = 0.0f;
        F32 new_val = 0.0f;
        ui_color_picker_sv(Str8FromC("SV picker"), ui_px(150), ui_px(150), hsv, &new_sat, &new_val);
        hsv.saturation = new_sat;
        hsv.value      = new_val;

        r_clear_frame_buffer(rgb_from_hsv(hsv));

        ui_spacer(ui_px(25));

        UI_Col()
        {
          // ui_label_f("hue: %f", new_hue);
          // ui_label_f("%f", final_color_as_hsv.r);
          // ui_label_f("%f", final_color_as_hsv.g);
          // ui_label_f("%f", final_color_as_hsv.b);
          // ui_label_f("hue: %f", new_hue);
          // ui_label_f("%f", rgb_from_hsv(final_color_as_hsv).r);//(U32)(final_color.r * 255.0f));
          // ui_label_f("%f", rgb_from_hsv(final_color_as_hsv).g);//(U32)(final_color.g * 255.0f));
          // ui_label_f("%f", rgb_from_hsv(final_color_as_hsv).b);//(U32)(final_color.b * 255.0f));
        }

        // final_color.r = new_final_color.r;
        // final_color.g = new_final_color.g;
        // final_color.b = new_final_color.b;

        // todo: HSV vertical slider picker
        // todo: rgb picker based on the hsv value
        // todo: slider for red color value
        // todo: slider for green color value
        // todo: slider for blue color value
        // todo: slider for alpha color value
      }

      // todo: Eraser button
    }
  }

  ui_end_build();

  ProfileFuncEnd();
}

void pencil_render(const Pencil_state* P)
{
  ID3D11Texture2D* texture, Rect dest_rect, Rect src_rect, B32 is_text, V4F32 text_color;
  d_add_texture_command()

  ID3D11RenderTargetView* frame_buffer_rtv = r_get_frame_buffer_rtv();
  Rect rect = rect_make(0, 0, r_get_texture_dims(P->draw_texture_always_fresh).x, r_get_texture_dims(P->draw_texture_always_fresh).y);
  r_draw_texture(
    frame_buffer_rtv, rect,
    P->draw_texture_always_fresh, rect
  );
  frame_buffer_rtv->Release();

  // InvalidCodePath();
}

#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P)
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
    r_export_texture(P->draw_texture_always_fresh, Str8FromC("always_fresh_texture.png"));
  }

  // Loading up not_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->draw_texture_not_that_fresh, Str8FromC("not_always_fresh_texture.png"));
  }
  
  // Loading up current texture_after_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_after_we_affected, Str8FromC("current_texture_after_we_affected.png"));
  }

  // Loading up current texture_before_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_before_we_affected, Str8FromC("current_texture_before_we_affected.png"));
  }
}
#endif

#endif