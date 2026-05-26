#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "pencil.h"

///////////////////////////////////////////////////////////
// - Main passes
//
void pencil_init(Pencil_state* P)
{
  P->frame_arena = arena_alloc(Megabytes(64));

  P->pen_size       = 10;
  P->pen_color_hsva = hsva_from_rgba(yellow());
  P->eraser_size    = 20;

  P->draw_texures_width  = (U32)os_get_client_area_dims__unsynched().x; // todo: Handle the case when the area is negative
  P->draw_texures_height = (U32)os_get_client_area_dims__unsynched().y; // todo: Handle the case when the area is negative

  P->draw_texture_always_fresh = r_make_texture(P->draw_texures_width, P->draw_texures_height);

  // Putting everything into the free list since we already have a static buffer of draw records
  for EachIndex(i, DRAW_RECORDS_MAX_COUNT)
  {
    Draw_record* record = P->pool_of_draw_records + i;
    DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
  }

  // Draw_record_registration_result record = register_new_draw_record(P);
}

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse, B32 is_ruler_mode)
{
  // Assert(NAND(is_ui_capturing_mouse, P->is_mid_drawing));

  // Handling signals (Right now only 1 per frame)
  {
    if (P->signal_swap_to_draw)
    {
      // todo: We should have a more legic check for beeing in the middle of a thing like drawing or ruling
      if (!P->is_mid_ruling && P->current_mode != Pencil_mode__draw)
      {
        P->current_mode = Pencil_mode__draw;
        // todo: Rest the stuff that dont have to exist cross modes
      }
      P->signal_swap_to_draw = false; 
    }
    else 
    if (P->signal_swap_to_ruler)
    {
      // todo: We should have a more legic check for beeing in the middle of a thing like drawing or ruling
      if (!P->is_mid_drawing && P->current_mode != Pencil_mode__ruler)
      {
        P->current_mode     = Pencil_mode__ruler;
        P->is_mid_ruling    = false;
        P->ruling_start_pos = V2F32{};
        P->ruling_end_pos   = V2F32{};
        // todo: Rest the stuff that dont have to exist cross modes
      }
      P->signal_swap_to_ruler = false;
    }
    else 
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
    else 
    if (P->signal_new_pen_color_hsva)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->signal_new_pen_color_hsva = false;
        P->pen_color_hsva = P->new_pen_color_hsva;
      }
    }    
  }

  if (is_ui_capturing_mouse) { return; }

  if (P->current_mode == Pencil_mode__draw)
  {
    B32 dont_start_drawing_this_frame = false;
  
    // This is fine to do and keep doing the rest of the frmae update since this is not dependand on anything and can just be plugged in
    if (os_wheel_got_scrolled())
    {
      S64 scroll = (S64)os_get_wheel_scroll();
      S64 current_pen_size = (S64)P->pen_size;
      current_pen_size += scroll;
      clamp_s64_inplace(&current_pen_size, (S64)MIN_PEN_SIZE, (S64)MAX_PEN_SIZE);
      P->pen_size = (U32)current_pen_size;
    }
    
    if (!P->is_mid_drawing && os_key_down(Key__Control) && os_key_down(Key__Shift) && (os_key_went_down(Key__Z) || os_key_repeat_down(Key__Z))) 
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
        r_copy_into_texture_from_texture(P->draw_texture_always_fresh, next_record->texture_after_we_affected, 0);
        P->current_record = next_record;
      }
    }
    else // User wants to remove the last line they drew
    if (!P->is_mid_drawing && os_key_down(Key__Control) && (os_key_went_down(Key__Z) || os_key_repeat_down(Key__Z))) 
    {
      dont_start_drawing_this_frame = true;
  
      if (P->current_record != 0)
      {
        Draw_record* record = P->current_record;
        r_copy_into_texture_from_texture(P->draw_texture_always_fresh, record->texture_before_we_affected, 0);
        P->current_record = P->current_record->prev;
      }
    }
    else // User want to clear the screen
    if (!P->is_mid_drawing && os_key_went_up(Key__Delete))
    {
      dont_start_drawing_this_frame = true;
  
      // Creating a new current record
      Draw_record_registration_result record_reg = register_new_draw_record(P);
      if (record_reg.succ)
      {
        P->current_record = record_reg.record;
      
        U64 w = P->draw_texures_width;
        U64 h = P->draw_texures_height;
        
        // Storing the texture before we clear it
        r_copy_into_texture_from_texture(P->current_record->texture_before_we_affected, P->draw_texture_always_fresh, 0);
  
        // Clearing the texture right here (not waiting for batching)
        r_clear_target(P->draw_texture_always_fresh, transparent());
        
        // Storing the texture after clearing it 
        r_copy_into_texture_from_texture(P->current_record->texture_after_we_affected, P->draw_texture_always_fresh, 0);
      }
    }
    else // User wants to start using the eraser pen 
    if (!P->is_mid_drawing && os_key_went_down(Key__E))
    {
      // note: 
      // There might be a sligh delay here, since we check the is_mid_drawing == false, but it might get
      // changed after the draw update loop, so i could make an event here to after the loop execute the frame event,
      // but right now its fine, i tested it, and delay is fine. I cant press buttons that fast to notice it. 
      // This was written on (18th May 2026)
      dont_start_drawing_this_frame = true;
      P->is_erasing_mode = true;
    }
    else // User wants to start using the brush/pen
    if (!P->is_mid_drawing && os_key_went_down(Key__B))
    {
      // note: 
      // There might be a sligh delay here, since we check the is_mid_drawing == false, but it might get
      // changed after the draw update loop, so i could make an event here to after the loop execute the frame event,
      // but right now its fine, i tested it, and delay is fine. I cant press buttons that fast to notice it. 
      // This was written on (18th May 2026)
      dont_start_drawing_this_frame = true;
      P->is_erasing_mode = false;
      // end_frame_event_swap_to_pen = true; 
    } 
    else 
    if (os_key_went_down(Key__Tab))
    {
      P->show_brush_ui_menu = ToggleBool(P->show_brush_ui_menu);
    }
  
    if (dont_start_drawing_this_frame) { goto __active_draw_update_routine_end__; }
  
    // Starting a new draw record
    if (!P->is_mid_drawing && os_mouse_button_down(Mouse_button__left)) 
    {
      Draw_record_registration_result record_registation = register_new_draw_record(P);
      if (record_registation.succ)
      {
        P->is_mid_drawing = true;
        P->current_record = record_registation.record;
      }
    }
    else // Updating active drawing 
    if (P->is_mid_drawing && os_mouse_button_down(Mouse_button__left))
    {
      V2F32 new_pos  = os_get_mouse_pos();
      V2F32 prev_pos = os_get_prev_mouse_pos();
      
      V4F32 color_rgba = rgba_from_hsva(P->pen_color_hsva);
      F32 pen_size = (F32)P->pen_size;
      
      if (P->is_erasing_mode) { 
        d_push_blend_kind(R_Blend_kind__no_blend);
        color_rgba = transparent(); 
        pen_size = (F32)P->eraser_size; 
      }
  
      D_RenderTarget(P->draw_texture_always_fresh)
      {
        F32 dx     = new_pos.x - prev_pos.x;
        F32 dy     = new_pos.y - prev_pos.y;
        F32 length = sqrtf(dx * dx + dy * dy);
        U64 steps  = (U64)length;
  
        // Drawing a continuos line based on delta
        for (U64 i = 0; i <= steps; i++) 
        {
          F32 t = (steps == 0) ? 0.0f : (F32)i / steps;
          F32 x = prev_pos.x + dx * t;
          F32 y = prev_pos.y + dy * t;
          V4F32 corner_colors[UV__COUNT] = { color_rgba, color_rgba, color_rgba, color_rgba };
          d_draw_circle(v2f32(x, y), pen_size, color_rgba, 0.0f);
        }
      }
      if (P->is_erasing_mode) { d_pop_blend_kind(); }
    }
    else // Here we finalise the draw record that the user have been drawing
    if (P->is_mid_drawing && os_mouse_button_went_up(Mouse_button__left))
    {
      Assert(P->current_record != 0);
      Assert(!r_target_match(r_target_zero_handle(), P->current_record->texture_after_we_affected));  // These are expected to already be allocated by this point
      Assert(!r_target_match(r_target_zero_handle(), P->current_record->texture_before_we_affected)); // These are expected to already be allocated by this point
  
      P->is_mid_drawing = false;
  
      // note: By this point, the fresh draw texture is the new final version of what the user has draw
      Draw_record* record = P->current_record;
  
      // Storing the new version of the draw texture
      r_copy_into_texture_from_texture(record->texture_after_we_affected, P->draw_texture_always_fresh, 0);
    }
  
    __active_draw_update_routine_end__: {};
  }
  else 
  if (P->current_mode == Pencil_mode__ruler)
  {
    B32 end_ruling = false;

    if (!P->is_mid_ruling && os_mouse_button_went_down(Mouse_button__left))
    {
      P->is_mid_ruling    = true;
      P->ruling_start_pos = os_get_mouse_pos();
      P->ruling_end_pos   = os_get_mouse_pos();
      P->ruling_mode      = Pencil_ruling_mode__not_set;
    }
    else // Figuring out how we gon go about user using the ruller, do we want them to hold the mouse button or press it to start and end  
    if (P->is_mid_ruling && P->ruling_mode == Pencil_ruling_mode__not_set)
    {
      if (os_mouse_button_up(Mouse_button__left) && v2f32_match(P->ruling_start_pos, os_get_mouse_pos())) { 
        P->ruling_mode = Pencil_ruling_mode__single_press;
      }
      else if (os_mouse_button_down(Mouse_button__left) && !v2f32_match(P->ruling_start_pos, os_get_mouse_pos())) {
        P->ruling_mode = Pencil_ruling_mode__hold;
      }
    }
    else // Updating the ruling 
    if (P->is_mid_ruling && P->ruling_mode != Pencil_ruling_mode__not_set)
    {
      // Condition to end the ruling
      if (   P->ruling_mode == Pencil_ruling_mode__hold         && os_mouse_button_up(Mouse_button__left)
          || P->ruling_mode == Pencil_ruling_mode__single_press && os_mouse_button_went_up(Mouse_button__left)
      ) {
        P->is_mid_ruling = false;
      } 
      else {
        P->ruling_end_pos = os_get_mouse_pos();
      }
    }
  }
}

void pencil_render(const Pencil_state* P)
{
  // Rendering the drawings
  {
    Rect rect = {};
    rect.width  = (F32)P->draw_texures_width;
    rect.height = (F32)P->draw_texures_height;
    d_add_texture_command(P->draw_texture_always_fresh, rect, rect, white());
  }

  // Rendering the ruller
  {
    if (P->current_mode == Pencil_mode__ruler)
    {
      Rect ruler_rect = rect_from_range_v2f32(range_v2f32_as_bb(P->ruling_start_pos, P->ruling_end_pos));
      d_draw_rect_inset_borders(ruler_rect, red(), 2.0f, v4f32_all(0.0f), 0.0f);
  
      V2F32 ruler_rect_center = rect_get_center(ruler_rect);
      V2F32 reler_dims        = rect_get_dims(ruler_rect);
      FP_Font font            = ui_get_font();
      V2F32 text_pos          = v2f32_sub(ruler_rect_center, v2f32(0.0f, (fp_get_font_height(font) / 2.0f))); 
  
      d_draw_circle(ruler_rect_center, 3, green(), 2.0f);
      d_draw_text_f("W: %.0f, H: %.0f", font, text_pos, white(), reler_dims.x, reler_dims.y);
    }
  }


  // os_show_cursor(false);
  // d_draw_circle_inset_border(os_get_mouse_pos(), (F32)P->pen_size, red(), 1.0f, 0.0f);
  // os_show_cursor(true);

  ///

}

void pencil_do_ui(Pencil_state* P, FP_Font font)
{ 
  V4F32 pen_color_rgba = rgba_from_hsva(P->pen_color_hsva);

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos());

  ui_push_font(font);

  ui_set_next_size_x(ui_p_of_p(1.0f, 1.0f));
  ui_set_next_size_y(ui_p_of_p(1.0f, 1.0f));
  ui_set_next_border(20, red());
  ui_set_next_softness(0.0f);
  UI_Box* content_inner = ui_box_make(Str8FromC("test id"), UI_Box_flag__has_borders);

  static F32 x_offset = 0.0f;
  static F32 y_offset = 0.0f;
  static F32 inside_menu_mouse_rel_x = 0.0f;
  static F32 inside_menu_mouse_rel_y = 0.0f;

  UI_Parent(content_inner)
  {
    UI_Row()
    {
      ui_spacer(ui_px(x_offset));

      UI_Col()
      {
        ui_spacer(ui_px(y_offset));
       
        if (P->show_brush_ui_menu)
        {
          ui_set_next_size_x(ui_children_sum());
          ui_set_next_size_y(ui_children_sum());
          ui_set_next_b_color(taupe());
          UI_Box* brush_box = ui_box_make(Str8FromC("Brush box menu id"), UI_Box_flag__has_background);
  
          UI_Parent(brush_box)
          {
            Str8 menu_name_box_id = Str8FromC("Brush box menu name box id");
            UI_Actions menu_name_actions = ui_actions_from_id(menu_name_box_id);
          
            if (!menu_name_actions.was_down && menu_name_actions.is_down)
            {
              UI_Box_data content_inner_data = ui_get_box_data_prev_frame_from_box(content_inner);
              V2F32 content_inner_p = rect_get_origin(content_inner_data.on_screen_rect);
    
              inside_menu_mouse_rel_x = (ui_get_mouse_x() - content_inner_p.x) - x_offset;
              inside_menu_mouse_rel_y = (ui_get_mouse_y() - content_inner_p.y) - y_offset;
            }
            else if (menu_name_actions.is_down)
            {
              UI_Box_data content_inner_box = ui_get_box_data_prev_frame_from_box(content_inner);
              x_offset = ui_get_mouse_x() - content_inner_box.on_screen_rect.x - inside_menu_mouse_rel_x;
              y_offset = ui_get_mouse_y() - content_inner_box.on_screen_rect.y - inside_menu_mouse_rel_y;
            }
            else {
              inside_menu_mouse_rel_x = 0.0f;
              inside_menu_mouse_rel_y = 0.0f;
            }
  
            // if (menu_name_actions.is_hovered) { ui_set_cursor(OS_Cursor__hand); }
            ui_set_next_size_x(ui_px(200));
            ui_set_next_size_y(ui_px(40));
            ui_set_next_layout_axis(Axis2__x);
            if (menu_name_actions.is_hovered) { ui_set_next_b_color(v4f32(0.1f, 0.5f, 0.6f, 1.0f)); } else { ui_set_next_b_color(blue()); }
            UI_Box* menu_name_box = ui_box_make(Str8FromC("Brush box menu name box id"), UI_Box_flag__has_background);
  
  
            UI_Parent(menu_name_box)
            {
              UI_PaddedBoxEx(ui_px(7), ui_px(0), ui_p_of_p(1, 0), ui_p_of_p(1, 0), Axis2__x)
              {
                ui_label(Str8FromC("Brushes"));
              }
            }
  
            ui_spacer(ui_px(7));
            
            if (!P->is_erasing_mode)  
            {
              ui_set_next_size_x(ui_px(200));
              ui_set_next_size_y(ui_px(40 + 14));
              ui_set_next_layout_axis(Axis2__x);
              UI_Box* pen_size_box = ui_box_make(Str8FromC("Box with color change_ui"), 0);
  
              UI_Parent(pen_size_box)
              {
                UI_PaddedBoxEx(ui_px(7), ui_px(7), ui_p_of_p(1, 0), ui_p_of_p(1, 0), Axis2__x)
                {
                  UI_Slider_style slider_style = {};
                  slider_style.size_x = ui_px(100);//ui_p_of_p(1.0, 0.0);
                  slider_style.size_y = ui_px(50);//ui_px(40);
                  slider_style.fmt_str = "%.0f";
                  slider_style.slided_part_color = v4f32(0.37f, 0.43f, 0.39f, 1.0f);
                  slider_style.no_hover_color = v4f32(0.5f, 0.5f, 0.5f, 1.0f);
                  slider_style.hover_color = v4f32(0.53f, 0.53f, 0.53f, 1.0f);
    
                  F32 new_pen_size = 0.0f;
                  B32 interacted = ui_slider(Str8FromC("Pen size slider id"), &slider_style, (F32)P->pen_size, 1.0f, 100.0f, &new_pen_size);
                  if (interacted)
                  {
                    P->signal_new_pen_size = true;
                    P->new_pen_size = (U32)new_pen_size;
                  }
  
                  ui_spacer(ui_px(7));
                  
                  ui_label_f("Pen size");
                }
              }
  
              ui_set_next_size_x(ui_px(200));
              ui_set_next_size_y(ui_px(200));
              UI_Box* box_with_color_ui = ui_box_make(Str8FromC("Box with color change_ui"), 0);
  
              UI_Parent(box_with_color_ui)
              UI_PaddedBox(ui_px(7), Axis2__x)
              {
                UI_Col()
                {
                  UI_Row()
                  {
                    F32 new_hsv = 0.0f;
                    ui_color_picker_h(Str8FromC("Hue picker"), ui_p_of_p(1.0f, 0.1f), ui_p_of_p(1.0f, 0.3f), Axis2__y, P->pen_color_hsva.hue, &new_hsv);
              
                    ui_spacer(ui_px(10));
              
                    F32 new_sat = 0.0f;
                    F32 new_val = 0.0f;
                    ui_color_picker_sv(Str8FromC("SV picker"), ui_p_of_p(1.0f, 0.2f), ui_p_of_p(1.0f, 0.3f), P->pen_color_hsva, &new_sat, &new_val);
              
                    V4F32 new_color_hsv = v4f32(new_hsv, new_sat, new_val, P->pen_color_hsva.a);
                    if (!v4f32_match(new_color_hsv, P->pen_color_hsva))
                    {
                      P->signal_new_pen_color_hsva = true;
                      P->new_pen_color_hsva = new_color_hsv;
                    }
            
                    ui_spacer(ui_px(10));
            
                    ui_set_next_size_x(ui_p_of_p(1.0f, 0.2f));
                    ui_set_next_size_y(ui_p_of_p(1.0f, 0.3f));
                    ui_set_next_layout_axis(Axis2__y);
                    UI_Parent(ui_box_make(Str8{}, 0))
                    {
                      UI_Slider_style slider_style = {};
                      slider_style.size_x = ui_p_of_p(1.0f, 0.1f);
                      slider_style.size_y = ui_p_of_p(1.0f, 0.1f);
                      slider_style.fmt_str = "%.0f";
                      slider_style.slided_part_color = v4f32(0.37f, 0.43f, 0.39f, 1.0f);
                      slider_style.no_hover_color = v4f32(0.5f, 0.5f, 0.5f, 1.0f);
                      slider_style.hover_color = v4f32(0.53f, 0.53f, 0.53f, 1.0f);
    
                      V4F32 new_rga_value_0_255 = {};
                      new_rga_value_0_255.r = pen_color_rgba.r * 255.0f;
                      new_rga_value_0_255.g = pen_color_rgba.g * 255.0f;
                      new_rga_value_0_255.b = pen_color_rgba.b * 255.0f;
                      new_rga_value_0_255.a = pen_color_rgba.a * 255.0f;
            
                      B32 interacted = false;
            
                      F32 new_r = 0.0f, new_g = 0.0f, new_b = 0.0f, new_a = 0.0f;
                      
                      slider_style.fmt_str = "R: %0.0f";
                      interacted |= ui_slider(Str8FromC("Slider for red color id"),   &slider_style, pen_color_rgba.r * 255.0f, 0.0f, 255.0f, &new_r);
                      ui_spacer(ui_px(10));
                      slider_style.fmt_str = "G: %0.0f";
                      interacted |= ui_slider(Str8FromC("Slider for green color id"), &slider_style, pen_color_rgba.g * 255.0f, 0.0f, 255.0f, &new_g);
                      ui_spacer(ui_px(10));
                      slider_style.fmt_str = "B: %0.0f";
                      interacted |= ui_slider(Str8FromC("Slider for blue color id"),  &slider_style, pen_color_rgba.b * 255.0f, 0.0f, 255.0f, &new_b);
                      ui_spacer(ui_px(10));
                      slider_style.fmt_str = "A: %0.0f";
                      interacted |= ui_slider(Str8FromC("Slider for alpha color id"), &slider_style, pen_color_rgba.a * 255.0f, 0.0f, 255.0f, &new_a);
            
                      new_rga_value_0_255.r = new_r;
                      new_rga_value_0_255.g = new_g;
                      new_rga_value_0_255.b = new_b;
                      new_rga_value_0_255.a = new_a;
            
                      new_rga_value_0_255.r /= 255.0f;
                      new_rga_value_0_255.g /= 255.0f;
                      new_rga_value_0_255.b /= 255.0f;
                      new_rga_value_0_255.a /= 255.0f;
            
                      if (interacted)
                      {
                        V4F32 new_hsva = hsva_from_rgba(new_rga_value_0_255);
                        P->signal_new_pen_color_hsva = true;
                        P->new_pen_color_hsva = new_hsva;
                      }        
                    }
                  }
        
                  ui_spacer(ui_px(10));
        
                  ui_set_next_size_x(ui_p_of_p(1.0f, 0.1f));
                  ui_set_next_size_y(ui_p_of_p(0.05f, 0.0f));
                  ui_set_next_b_color(pen_color_rgba);
                  UI_Box* color_rect_box = ui_box_make(Str8{}, UI_Box_flag__has_background);
                }
              }
    
              UI_PaddedBoxEx(ui_px(7), ui_px(7), ui_px(0), ui_px(0), Axis2__x)
              {
                Str8 button_id = Str8FromC("Eraser##Button id");
                UI_Actions actions = ui_actions_from_id(button_id);
                if (actions.is_down)         { ui_set_next_b_color(v4f32(0.573f, 0.169f, 0.129f, 1.0f)); }
                else if (actions.is_hovered) { ui_set_next_b_color(v4f32(1.0f, 0.420f, 0.420f, 1.0f)); }
                else                         { ui_set_next_b_color(v4f32(0.753f, 0.224f, 0.169f, 1.0f)); }
                ui_set_next_corner_r(v4f32_all(0.25f));
                UI_Box* button_box = ui_box_make(button_id, UI_Box_flag__has_background|UI_Box_flag__has_rounded_corners);
  
                UI_Parent(button_box)
                UI_PaddedBox(ui_px(10), Axis2__x)
                {
                  ui_label_f("Eraser");
                }
  
                if (actions.is_clicked)
                {
                  P->signal_swap_to_eraser = true;
                }
              }
  
              ui_spacer(ui_px(7));
            }
            else 
            {
              UI_PaddedBox(ui_px(7), Axis2__y)
              {
                // Eraser size slider + text next to it
                ui_label_f("Eraser size");
                
                ui_spacer(ui_px(3));
                UI_Slider_style slider_style = {};
                slider_style.size_x = ui_px(60);
                slider_style.size_y = ui_px(40);
                slider_style.fmt_str = "%.0f";
                slider_style.slided_part_color = v4f32(0.37f, 0.43f, 0.39f, 1.0f);
                slider_style.no_hover_color = v4f32(0.5f, 0.5f, 0.5f, 1.0f);
                slider_style.hover_color = v4f32(0.53f, 0.53f, 0.53f, 1.0f);
  
                F32 new_eraser_size = 0.0f;
                B32 interacted = ui_slider(Str8FromC("Slider for eraser size id"), &slider_style, (F32)P->eraser_size, 1.0f, 100.0f, &new_eraser_size);
                if (interacted)
                {
                  P->signal_new_eraser_size = true;
                  P->new_eraser_size = (U32)new_eraser_size;
                }
  
                ui_spacer(ui_px(7));
  
                {
                  Str8 button_id = Str8FromC("Brush##Button id");
                  UI_Actions actions = ui_actions_from_id(button_id);
                  if (actions.is_down)         { ui_set_next_b_color(v4f32(0.573f, 0.169f, 0.129f, 1.0f)); }
                  else if (actions.is_hovered) { ui_set_next_b_color(v4f32(1.0f, 0.420f, 0.420f, 1.0f)); }
                  else                         { ui_set_next_b_color(v4f32(0.753f, 0.224f, 0.169f, 1.0f)); }
                  ui_set_next_corner_r(v4f32_all(0.25f));
                  UI_Box* button_box = ui_box_make(button_id, UI_Box_flag__has_background|UI_Box_flag__has_rounded_corners);
  
                  UI_Parent(button_box)
                  UI_PaddedBox(ui_px(10), Axis2__x)
                  {
                    ui_label_f("Brush");
                  }
  
                  if (actions.is_clicked)
                  {
                    P->signal_swap_to_pen = true;
                  }
                }
  
              }
            }
              
          }
        }

      }
    }
  }

  ui_end_build();
}

///////////////////////////////////////////////////////////
// - Other
//
// todo: I dont know how i feel about this having positioning logic for the list and the name is not showing that
Draw_record_registration_result register_new_draw_record(Pencil_state* P)
{
  // note:
  // This routine is not used a lot, but used and is very important to how the state manages the order of
  // draw records. The issue with it, is that the state does not clear frame based bounds for drawing.
  // The bounds are cross frame. For this reason i cant have a begin_draw/end_draw api. I cant have a no op 
  // as well here for reasons. Just assert is bad here since then in release if i mess up, the app will
  // have invalid state, which is really bad, i would rather noting happend and not invalid state.
  // I dont like the optional api that much, but since this routine is called right now only 2 times (Today is 18th of May 2026), 
  // its fine. And aslo removes all the knolage from the caller about when this routine is legal to be called
  // and leaves that knolage to be used to the routine itself. 
  if (P->is_mid_drawing) { InvalidCodePath(); return Draw_record_registration_result{}; }

  // Freeing all the records that are in front of the current one
  {
    // todo: I feel like releasing them here is fine, but i could also just reuse them since they are all the same size
    //       This would then also mean that i can just prealloc all of them at startup and just reuse by clearing them.
    //       This would also mean that i can test how many i can allocate up to an upper bound also at the startup. 
    //       Hm. If this is possible then this shoud be way better, BUT, this might not work when we have 
    //       handling for screen or task bar resize, which shoud be handled, but for now isnt, so look into this
    //       when it is.
    for (Draw_record* record = P->last_record;;) 
    {
      if (record == P->current_record) { break; }
      
      r_release_texture(&record->texture_before_we_affected);
      r_release_texture(&record->texture_after_we_affected);
      
      DllPopBack_Name(P, first_record, last_record, next, prev);
      Draw_record* prev_record = record->prev;
      
      *record = Draw_record{};
      DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
      
      record = prev_record;
    }
  }

  Draw_record* new_draw_record = __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(P);
  if (new_draw_record == 0)
  {
    Draw_record* oldest_record = P->first_record;
    DllPopFront_Name(P, first_record, last_record, next, prev);
    
    *oldest_record = Draw_record{};
    DllPushBack_Name(P, oldest_record, first_free_draw_record, last_free_draw_record, next, prev);

    new_draw_record = __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(P);
  }
  Assert(new_draw_record != 0); 

  // Adding the new draw record to the draw record queue
  DllPushBack_Name(P, new_draw_record, first_record, last_record, next, prev); Assert(P->last_record == new_draw_record);

  new_draw_record->texture_before_we_affected = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  // todo:
  // HandleLater(new_draw_record->texture_before_we_affected != 0);

  r_copy_into_texture_from_texture(new_draw_record->texture_before_we_affected, P->draw_texture_always_fresh, 0);

  new_draw_record->texture_after_we_affected = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  // todo:
  // HandleLater(new_draw_record->texture_after_we_affected != 0);

  Draw_record_registration_result result = {};
  result.succ = true;
  result.record = new_draw_record;

  return result;
}

// note: This is private for register_new_draw_record
Draw_record* __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(Pencil_state* P)
{
  Assert(!P->is_mid_drawing);

  Draw_record* result = 0;
  if (P->first_free_draw_record)
  {
    result = P->first_free_draw_record;
    DllPopFront_Name(P, first_free_draw_record, last_free_draw_record, next, prev);
    *result = Draw_record{};
  }
  return result;
}

void add_shortcut(Pencil_state* P, Key_modifier mod, Key key, Str8 command_name)
{
  Shortcut_chord* chord = 0;

  // Might have the command or chord already set, so we will reset it 
  for (U64 i = 0; i < P->chord_count; i += 1)
  {
    Shortcut_chord* test_chord = P->chords + i;
    if (   str8_match(test_chord->command_name, command_name, 0) 
        || (test_chord->mod == mod && test_chord->key == key)
    ) { 
      chord = test_chord;
      break;
    }
  }

  // No chord to reset, getting a new one
  if (chord) { InvalidCodePath(); } // I dont want to handle this right now, since i dont have dynamic chords
  else {
    if (P->chord_count < MAX_CHORD_COUNT) {
      chord = P->chords + (P->chord_count++);
    }
    if (chord == 0) { InvalidCodePath(); } // Need more space for chords
    else {
      chord->mod = mod;
      chord->key = key;
      chord->command_name = command_name;
    }
  }
}

void run_command_from_name(Pencil_state* P, Str8 command_name)
{
  if (0) {}
  else if (str8_match(command_name, COMMAND_NAME_TERMINATE_APP, 0)) { command_terminate_app(P); }
  else if (str8_match(command_name, COMMAND_NAME_SWAP_TO_RULER, 0)) { command_swap_to_ruller(P); }
  else if (str8_match(command_name, COMMAND_NAME_SWAP_TO_DRAW, 0))  { command_swap_to_draw(P); }
  else {
    InvalidCodePath();
  }
}

void command_terminate_app(Pencil_state* P)
{
  P->terminate_app = true;
}

void command_swap_to_ruller(Pencil_state* P)
{
  P->signal_swap_to_ruler = true;
}

void command_swap_to_draw(Pencil_state* P)
{
  P->signal_swap_to_draw = true;
}

// todo: I would like to pass P here as const, and signals as a separate thing then to have it clear that ui doesnt modify the state at all
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

  // Loading up current texture_after_we_affected_rtv
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_after_we_affected, Str8FromC("current_texture_after_we_affected.png"));
  }

  // Loading up current texture_before_we_affected_rtv
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_before_we_affected, Str8FromC("current_texture_before_we_affected.png"));
  }
}
#endif

#endif