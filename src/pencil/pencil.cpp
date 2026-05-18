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

  P->draw_texture_always_fresh     = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  P->draw_texture_always_fresh_rtv = r_rtv_from_texture(P->draw_texture_always_fresh);

  // Putting everything into the free list since we already have a static buffer of draw records
  for EachIndex(i, DRAW_RECORDS_MAX_COUNT)
  {
    Draw_record* record = P->pool_of_draw_records + i;
    DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
  }
}

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse)
{
  Assert(NAND(is_ui_capturing_mouse, P->is_mid_drawing));

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

  B32 dont_start_drawing_this_frame = false;

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
      ID3D11RenderTargetView* future_texture = next_record->texture_after_we_affected_rtv;
      r_copy_into_texture_from_texture(P->draw_texture_always_fresh_rtv, future_texture);
      P->current_record = next_record;
    }
  }
  else // User wants to remove the last line they drew
  if (!P->is_mid_drawing && os_key_down(Key__Control) && (os_key_went_down(Key__Z) || os_key_repeat_down(Key__Z))) 
  {
    dont_start_drawing_this_frame = true;

    // todo: This is always 0 for some reason, some wrong with the list, fix it dude
    if (P->current_record != 0)
    {
      Draw_record* record = P->current_record;
      ID3D11RenderTargetView* texture_before_we_affected_rtv = record->texture_before_we_affected_rtv;
      r_copy_into_texture_from_texture(P->draw_texture_always_fresh_rtv, texture_before_we_affected_rtv);
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
      r_copy_into_texture_from_texture(P->current_record->texture_before_we_affected_rtv, P->draw_texture_always_fresh_rtv);

      // Clearing the texture
      r_clear_rtv(P->draw_texture_always_fresh_rtv, transparent());
      
      // Storing the texture after clearing it
      r_copy_into_texture_from_texture(P->current_record->texture_after_we_affected_rtv, P->draw_texture_always_fresh_rtv);
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
  if (!P->is_mid_drawing && os_mouse_button_down(Mouse_button__Left)) 
  {
    Draw_record_registration_result record_registation = register_new_draw_record(P);
    if (record_registation.succ)
    {
      P->is_mid_drawing = true;
      P->current_record = record_registation.record;
    }
  }
  else // Updating active drawing 
  if (P->is_mid_drawing && os_mouse_button_down(Mouse_button__Left))
  {
    V2F32 new_pos  = os_get_mouse_pos();
    V2F32 prev_pos = os_get_prev_mouse_pos();
    
    V4F32 color_rgba = rgba_from_hsva(P->pen_color_hsva);
    F32 pen_size = (F32)P->pen_size;
    
    if (P->is_erasing_mode) { 
      d_push_blend_kind(D3D_Blend_kind__no_blend);
      color_rgba = transparent(); 
      pen_size = (F32)P->eraser_size; 
    }

    D_RenderTarget(P->draw_texture_always_fresh_rtv)
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
        x -= pen_size / 2.0f;
        y -= pen_size / 2.0f;
        d_add_rect_command_ex(rect_make(x, y, pen_size, pen_size), corner_colors, v4f32(1, 1, 1, 1), 0, 2.0f, {});
      }
    }
    if (P->is_erasing_mode) { d_pop_blend_kind(); }
  }
  else // Here we finalise the draw record that the user have been drawing
  if (P->is_mid_drawing && os_mouse_button_went_up(Mouse_button__Left))
  {
    Assert(P->current_record != 0);
    Assert(P->current_record->texture_after_we_affected_rtv != 0);  // These are expected to already be allocated by this point
    Assert(P->current_record->texture_before_we_affected_rtv != 0); // These are expected to already be allocated by this point

    P->is_mid_drawing = false;

    // note: By this point, the fresh draw texture is the new final version of what the user has draw
    Draw_record* record = P->current_record;

    // Storing the new version of the draw texture
    r_copy_into_texture_from_texture(record->texture_after_we_affected_rtv, P->draw_texture_always_fresh_rtv);
  }

  __active_draw_update_routine_end__: {};
}

void pencil_render(const Pencil_state* P)
{
  Rect rect = {};
  rect.width  = (F32)P->draw_texures_width;
  rect.height = (F32)P->draw_texures_height;
  // D_Bxlend(D3D_Blend_kind__no_blend) 
  {
    d_add_texture_command(P->draw_texture_always_fresh, rect, rect, false, V4F32{});
  }
}

void pencil_do_ui(Pencil_state* P, FP_Font font)
{ 
  ProfileFuncBegin();

  V4F32 pen_color_rgba = rgba_from_hsva(P->pen_color_hsva);

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos());

  ui_push_font(font);

  ui_set_next_size_x(ui_p_of_p(1.0f, 1.0f));
  ui_set_next_size_y(ui_p_of_p(1.0f, 1.0f));
  ui_set_next_border(10, red());
  ui_set_next_softness(0.0f);
  UI_Box* content_inner = ui_box_make(Str8{}, 0);

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
        
        ui_set_next_size_x(ui_children_sum());
        ui_set_next_size_y(ui_children_sum());
        ui_set_next_b_color(taupe());
        UI_Box* brush_box = ui_box_make(Str8FromC("Brush box menu id"), 0);

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
          if (menu_name_actions.is_hovered) { ui_set_next_b_color(v4f32(0.1f, 0.5f, 0.6f, 1.0f)); } else { ui_set_next_b_color(blue()); }
          ui_set_next_size_x(ui_px(200));
          ui_set_next_size_y(ui_px(40));
          ui_set_next_layout_axis(Axis2__x);
          UI_Box* menu_name_box = ui_box_make(Str8FromC("Brush box menu name box id"), 0);


          UI_Parent(menu_name_box)
          {
            UI_PaddedBoxEx(ui_px(7), ui_px(0), ui_p_of_p(1, 0), ui_p_of_p(1, 0), Axis2__x)
            {
              ui_label(Str8FromC("Brushes"));
            }
          }

          if (P->show_brush_ui_menu)
          {
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
                  slider_style.size_x = ui_p_of_p(1.0, 0.0);
                  slider_style.size_y = ui_px(40);
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
                      interacted |= ui_slider(Str8FromC("Slider for red color id"),   &slider_style, pen_color_rgba.r * 255.0f, 0.0f, 255.0f, &new_r);
                      ui_spacer(ui_px(10));
                      interacted |= ui_slider(Str8FromC("Slider for green color id"), &slider_style, pen_color_rgba.g * 255.0f, 0.0f, 255.0f, &new_g);
                      ui_spacer(ui_px(10));
                      interacted |= ui_slider(Str8FromC("Slider for blue color id"),  &slider_style, pen_color_rgba.b * 255.0f, 0.0f, 255.0f, &new_b);
                      ui_spacer(ui_px(10));
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
                  UI_Box* color_rect_box = ui_box_make(Str8{}, 0);
                }
              }
    
              UI_PaddedBoxEx(ui_px(7), ui_px(7), ui_px(0), ui_px(0), Axis2__x)
              {
                Str8 button_id = Str8FromC("Eraser##Button id");
                UI_Actions actions = ui_actions_from_id(button_id);
                if (actions.is_down)         { ui_set_next_b_color(v4f32(0.573f, 0.169f, 0.129f, 1.0f)); }
                else if (actions.is_hovered) { ui_set_next_b_color(v4f32(1.0f, 0.420f, 0.420f, 1.0f)); }
                else                         { ui_set_next_b_color(v4f32(0.753f, 0.224f, 0.169f, 1.0f)); }
                ui_set_next_corner_r(ui_corner_r_all(0.15f));
                UI_Box* button_box = ui_box_make(button_id, 0);

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
                  ui_set_next_corner_r(ui_corner_r_all(0.15f));
                  UI_Box* button_box = ui_box_make(button_id, 0);
  
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


    // else // Do the eraser menu
    // {
    //   ui_label_f("THIS AT SOME POINT WILL BE THE MENU FOR THE ERASER");
    // }
  }

  ui_end_build();

  ProfileFuncEnd();
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
  if (P->current_record != 0)
  {
    // todo: I feel like releasing them here is fine, but i could also just reuse them since they are all the same size
    //       This would then also mean that i can just prealloc all of them at startup and just reuse by clearing them.
    //       This would also mean that i can test how many i can allocate up to an upper bound also at the startup. 
    //       Hm. If this is possible then this shoud be way better, BUT, this might not work when we have 
    //       handling for screen or task bar resize, which shoud be handled, but for now isnt, so look into this
    //       when it is.
    for (Draw_record* record = P->last_record; record != 0;) 
    {
      if (record == P->current_record) { break; }
      record->texture_before_we_affected_rtv->Release();
      record->texture_after_we_affected_rtv->Release();
      
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

  new_draw_record->texture_before_we_affected     = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  new_draw_record->texture_before_we_affected_rtv = r_rtv_from_texture(new_draw_record->texture_before_we_affected);
  HandleLater(new_draw_record->texture_before_we_affected != 0);

  r_copy_into_texture_from_texture(new_draw_record->texture_before_we_affected_rtv, P->draw_texture_always_fresh_rtv);

  new_draw_record->texture_after_we_affected     = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  new_draw_record->texture_after_we_affected_rtv = r_rtv_from_texture(new_draw_record->texture_after_we_affected);
  HandleLater(new_draw_record->texture_after_we_affected != 0);

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
    r_export_texture(P->draw_texture_always_fresh_rtv, Str8FromC("always_fresh_texture.png"));
  }

  // Loading up current texture_after_we_affected_rtv
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_after_we_affected_rtv, Str8FromC("current_texture_after_we_affected.png"));
  }

  // Loading up current texture_before_we_affected_rtv
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    r_export_texture(P->current_record->texture_before_we_affected_rtv, Str8FromC("current_texture_before_we_affected.png"));
  }
}
#endif

#endif