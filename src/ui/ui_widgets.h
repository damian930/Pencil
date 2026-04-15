#ifndef __UI_WIDGETS_H
#define __UI_WIDGETS_H

#include "ui/ui_core.h"

// - Simple widgets
void ui_label_c(const char* c_str);
void ui_label(Str8 str);
void ui_label_fmt(const char* fmt, ...);
void ui_spacer(UI_Size size);
UI_Actions ui_button(Str8 str, RLI_Event_list* rli_events); // todo: Remove the fucking rli events from there dude

// - Layout stack
void ui_layout_stack_begin(Axis2 axis); // todo: Call these like a layout stack
void ui_layout_stack_end();
#define UI_XStack() DeferLoop(ui_layout_stack_begin(Axis2__x), ui_layout_stack_end())
#define UI_YStack() DeferLoop(ui_layout_stack_begin(Axis2__y), ui_layout_stack_end())

// - Padded box
void ui_padded_box_begin(UI_Size size, Axis2 final_box_axis);
void ui_padded_box_end(UI_Size size);
#define UI_PaddedBox(size, axis) DeferLoop(ui_padded_box_begin(size, axis), ui_padded_box_end(size))

// - Wrapper
void ui_wrapper_begin(Axis2 axis);
void ui_wrapper_end();
#define UI_Wrapper(axis) DeferLoop(ui_wrapper_begin(axis), ui_wrapper_end())

// - Slider
struct UI_Slider_style {
  F32 width;
  F32 height;
  F32 corner_r;
  const char* fmt_str;
  
  V4 hover_color;
  V4 no_hover_color;
  V4 slided_part_color;
  V4 text_color;
};
F32 ui_slider(Str8 slider_id, const UI_Slider_style* slider_style, F32 current_value, F32 min, F32 max, RLI_Event_list* rli_event_list)
{
  UI_Actions slider_actions = ui_actions_from_id(slider_id, rli_event_list);

  if (slider_actions.is_hovered) {
    ui_set_next_color(slider_style->hover_color);
  } else {
    ui_set_next_color(slider_style->no_hover_color);
  }
  ui_set_next_size_x(ui_px(slider_style->width));
  ui_set_next_size_y(ui_px(slider_style->height));
  ui_set_next_corner_radius(slider_style->corner_r);
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* slider_box = ui_box_make(slider_id, UI_Box_flag__dont_draw_overflow);

  B32 is_data_present = true;

  Rect slider_box_rect = {};
  {
    UI_Box_data slider_box_data = ui_get_box_data_prev_frame(slider_id);
    is_data_present &= slider_box_data.is_found;
    slider_box_rect = slider_box_data.on_screen_rect;
  }

  F32 thumb_container_width = slider_box_rect.width;
  F32 max_thumb_offset = thumb_container_width;
  F32 value_ratio = current_value / (max - min);
  clamp_f32_inplace(&value_ratio, 0.0f, 1.0f);
  F32 thumb_offset = max_thumb_offset * value_ratio;

  B32 moved_slider = false;
  if (slider_actions.is_down)
  {
    V2 mouse_pos = ui_get_mouse();
    thumb_offset = mouse_pos.x - slider_box_rect.x;
    moved_slider = true;
    ui_set_active(slider_box->id);
  }
  else 
  {
    ui_reset_active_match(slider_box->id);
  }
  clamp_f32_inplace(&thumb_offset, 0.0f, max_thumb_offset);

  UI_Parent(slider_box)
  {
    ui_set_next_size_x(ui_px(thumb_offset));
    ui_set_next_size_y(ui_px(slider_style->height));
    ui_set_next_corner_radius(slider_style->corner_r);
    ui_set_next_color(slider_style->slided_part_color);
    UI_Box* thumb_box = ui_box_make(Str8{}, 0);
  }

  UI_Parent(slider_box)
  {
    ui_set_next_flags(UI_Box_flag__floating);
    UI_PaddedBox(ui_p_of_p(1.0f, 0.0f), Axis2__y) UI_PaddedBox(ui_p_of_p(1.0f, 0.0f), Axis2__x)
    {
      ui_set_next_text_color(slider_style->text_color);
      ui_label_fmt(slider_style->fmt_str, current_value);
    }
  }

  F32 new_value = current_value;
  if (moved_slider)
  {
    new_value = lerp_f32(min, max, (thumb_offset / max_thumb_offset));
  }
  return new_value;
}

// - Text input field 
U64 __ui_move_with_control_left(Str8 str, U64 current_pos); 
U64 __ui_move_with_control_right(Str8 str, U64 current_pos);

enum UI_Text_op_kind {  
  UI_Text_op_kind__NONE,
  UI_Text_op_kind__move_cursor,
  UI_Text_op_kind__delete_section,
  UI_Text_op_kind__copy_section,
  UI_Text_op_kind__paste_at_cursor,
  UI_Text_op_kind__insert_char_at_cursor, 
};

enum UI_Text_op_move_specifier {
  UI_Text_op_move_specifier___NONE,
  UI_Text_op_move_specifier___move_1_char_left,                     
  UI_Text_op_move_specifier___move_1_char_right,                    
  UI_Text_op_move_specifier___move_1_word_left,                     
  UI_Text_op_move_specifier___move_1_word_right,                    
  UI_Text_op_move_specifier___move_to_line_start,                   
  UI_Text_op_move_specifier___move_to_line_end,                     
};

struct UI_Text_op {
  UI_Text_op_kind kind;

  // Fat struct data
  U8 char_to_insert;               
  B8 keep_section_start_after_op; 
  B8 dont_move_if_section;
  B8 override_move_and_move_to_section_min_if_ending_section;
  B8 override_move_and_move_to_section_max_if_ending_section;
  UI_Text_op_move_specifier move_specifier;

  UI_Text_op* next;
  UI_Text_op* prev;
};

struct UI_Text_op_list {
  UI_Text_op* first;
  UI_Text_op* last;
  U64 count;
};

UI_Text_op* ui_text_op_list_push(Arena* arena, UI_Text_op_list* list, UI_Text_op_kind kind);
UI_Text_op_list ui_text_op_list_from_rli_events(Arena* arena, RLI_Event_list* event_list);
void ui_aply_text_ops(UI_Text_op_list text_op_list, U8* text_buffer, U64 max_text_size, U64* current_text_size, U64* cursor_pos, U64* section_start);

///////////////////////////////////////////////////////////
// - Attempt to redo text edit box again (this shoud like like the text edit box for the search bar on youtube)
//
// struct UI_Text_edit_box_style {
//   F32 width_in_px;
//   Font font;
//   F32 font_size;
//   // V4 text_color;
//   // add: value that specifies how many simbols are minimum on a side to start changing clip offset
// };
// void ui_text_edit_box(
//   const UI_Text_edit_box_style* edit_box_style, 
//   U8* text_buffer, 
//   U64 text_buffer_max_size, // todo: I feel like for displaying this is not really needed 
//   U64 text_buffer_current_size, 
//   U64 cursor, 
//   U64 section,
//   RLI_Event_list* rli_events,
//   U64* left_wall_index,
//   U64* right_wall_index //,
//   // B32* is_first_time
// ) {
//   Str8 text_buffer_as_str = str8_manuall(text_buffer, text_buffer_current_size);

//   if (cursor > *right_wall_index) 
//   {
//     // note: cursor is more than 0 here always 
//     *right_wall_index = cursor;
    
//     F32 accumulated_text_width = 0.0f;
//     for (U64 test_left_wall_index = *right_wall_index - 1;; )
//     {
//       Str8 test_char_str = str8_substring(text_buffer_as_str, test_left_wall_index, test_left_wall_index + 1);
//       V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
//       accumulated_text_width += dims.x;
//       if (accumulated_text_width > edit_box_style->width_in_px)
//       {
//         *left_wall_index = test_left_wall_index + 1;
//         break;
//       }
//       if (test_left_wall_index == 0) { break; }
//       test_left_wall_index -= 1;
//     }
//   }
//   else if (cursor < *left_wall_index)
//   {
//     BreakPoint();
//   }

//   F32 height_in_px = 0.0f;
//   {
//     V2 dims = ui_measure_text_ex(Str8FromC(" "), edit_box_style->font, edit_box_style->font_size);
//     height_in_px = dims.y;
//   }

//   Str8 edit_box_id = Str8FromC("Edit box id");

//   ui_set_next_color({ 255, 255, 255, 255 });
//   ui_set_next_size_x(ui_px(edit_box_style->width_in_px));
//   ui_set_next_size_y(ui_px(height_in_px));
//   UI_Box* edit_box = ui_box_make(Str8FromC("Edit box id"), 0/*UI_Box_flag__dont_draw_overflow*/);

//   static B32 is_active = false;
//   UI_Actions edit_box_actions = ui_actions_from_box(edit_box, rli_events);
//   if (edit_box_actions.is_clicked && !is_active)
//   {
//     is_active = true; 
//   }

//   F32 clip_offset = 0.0f;
//   if (cursor == *right_wall_index)
//   {
//     Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, *right_wall_index + 1);
//     // Str8 str_between_walls   = str8_substring(text_buffer_as_str, *left_wall_index, *right_wall_index + 1);

//     V2 till_right_wall_dims = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
//     // V2 between_walls_dims    = ui_measure_text_ex(str_between_walls, edit_box_style->font, edit_box_style->font_size);

//     clip_offset = till_right_wall_dims.x - edit_box_style->width_in_px;
//     if (clip_offset < 0.0f) { clip_offset = 0.0f; }
//   }

//   edit_box->clip_offset.x = -1 * clip_offset;
//   UI_Parent(edit_box)
//   {
//     // Text 
//     ui_label(text_buffer_as_str);
  
//     // Cursor offset box
//     F32 cursor_width = 2.0f;
    
//     ui_set_next_size_x(ui_px(edit_box_style->width_in_px)); ui_set_next_size_y(ui_px(height_in_px));
//     ui_set_next_layout_axis(Axis2__x);
//     ui_set_next_flags(UI_Box_flag__floating);
//     UI_Box* cursor_offset_box = ui_box_make(Str8{}, 0);

//     F32 cursor_box_clip_offset = clip_offset + cursor_width;
//     if (cursor_box_clip_offset < 0.0f) { cursor_box_clip_offset = 0.0f; }
//     cursor_offset_box->clip_offset.x = -1 * cursor_box_clip_offset;
    
//     UI_Parent(cursor_offset_box)
//     {
//       Str8 str_before_cursor = str8_substring(text_buffer_as_str, 0, cursor);
//       F32 x_offset = ui_measure_text_ex(str_before_cursor, edit_box_style->font, edit_box_style->font_size).x;
//       ui_spacer(ui_px(x_offset));

//       ui_set_next_size_x(ui_px(cursor_width)); ui_set_next_size_y(ui_px(height_in_px));
//       ui_set_next_color({ 0, 0, 0, 255 });
//       UI_Box* cursor_line_box = ui_box_make(Str8{}, 0);
//     }

//     #define DRAW_DEBUG_WALLS 0
//     #if DRAW_DEBUG_WALLS
//     {
//       ui_set_next_size_x(ui_px(1)); ui_set_next_size_y(ui_px(height_in_px));
//       ui_set_next_layout_axis(Axis2__x);
//       ui_set_next_flags(UI_Box_flag__floating);
//       UI_Box* left_wall_offset_box = ui_box_make(Str8{}, 0);
//       left_wall_offset_box->clip_offset.x = -1 * (clip_offset + 1);
      
//       UI_Parent(left_wall_offset_box)
//       {
//         Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, *left_wall_index + 1);
//         V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//         ui_spacer(ui_px(dims.x));

//         ui_set_next_size_x(ui_px(1)); ui_set_next_size_y(ui_px(height_in_px));
//         ui_set_next_color({ 255, 0, 0, 255 });
//         UI_Box* left_wall = ui_box_make(Str8{}, 0);
//       }

//     }
//     #endif
//     #undef DRAW_DEBUG_WALLS
//   }

  






//   // When we go left we base it off of the left wall
//   // Whne we go right we base it off of the right wall

// }

struct UI_Text_edit_box_state {
  U64 left_bound;
  U64 right_bound;
};

// note: This is for viewing the thing only. All the data is from the outside
//       Also might be a good idea to call this text_box_read_only
B32 ui_text_edit_box_static(
  Str8 edit_box_id, 
  Str8 placeholder_str, 
  UI_Text_edit_box_state* edit_box_state, 
  UI_Size edit_box_size_x, 
  U8* text_buffer, 
  U64 buffer_max_size, 
  U64 buffer_current_size, 
  U64 cursor_pos, 
  U64 section_start_pos, 
  RLI_Event_list* rli_events
) {
  // todo: Make sure you understand the code here
  // todo: Change the logic for the bounds, they shoud be more like indexes, this makes more sence for the implementationimplementationimplementationimplementationimplementation
  // todo: Fix the bugs with this implementation
  //        - activeness bugs
  //        - disapering cursor bugs
  // todo: Be able to change the size of the cursor
  // todo: Add a way to spcify the number of string to still be visible after the bound, so the bound is no longer just the visible part of the screen

  clamp_u64_inplace(&cursor_pos, 0, buffer_max_size);
  clamp_u64_inplace(&section_start_pos, 0, buffer_max_size);
  
  Str8 buffer_str = str8_manuall(text_buffer, buffer_current_size);
  UI_Box_clip_data edit_box_clip_data = ui_get_box_clip_data_prev_frame(edit_box_id);
  F32 cursor_width = 2.0f;
  
  U64 left_bound  = edit_box_state->left_bound;
  U64 right_bound = edit_box_state->right_bound;

  // Calculating bounds if not valid
  if (edit_box_clip_data.is_found)
  {
    if (cursor_pos < left_bound) 
    {
      left_bound = cursor_pos;
      right_bound = cursor_pos; 

      // Calculate new other bound from the current one
      { 
        F32 total_width = 0.0f;
        for (;;)
        {
          if (right_bound == buffer_str.count) { break; }
          Str8 char_str = str8_manuall(text_buffer + right_bound, 1);
          F32 char_width = ui_measure_text(char_str).x;
          F32 test_width = total_width + char_width;
          if (test_width >= edit_box_clip_data.on_screen_dims.x) { break; } 
          else {
            right_bound += 1;
            total_width = test_width;
          }
        }
      }

      F32 width_before_left_bound = 0.0f;
      {
        Str8 str = str8_substring(buffer_str, 0, left_bound);
        width_before_left_bound = ui_measure_text(str).x;
      }

      F32* text_clip_x = &edit_box_clip_data.clip_offset->x;
      *text_clip_x = -width_before_left_bound;
    }
    else if (cursor_pos > (right_bound + 1)) 
    {
      right_bound = cursor_pos - 1; // -1 here since right bound is inclusive and cursor is not
      left_bound = cursor_pos - 1;

      // Calculate new  bound from the current one
      { 
        F32 total_width = 0.0f;
        for (;;)
        {
          if (left_bound == 0) { break; }
          Str8 char_str = str8_manuall(text_buffer + left_bound, 1);
          F32 char_width = ui_measure_text(char_str).x;
          F32 test_width = total_width + char_width;
          if (test_width >= edit_box_clip_data.on_screen_dims.x) { break; }
          else { 
            left_bound -= 1; 
            total_width = test_width;
          }
        }
      }

      // New clipping
      F32 width_before_right_bound = 0.0f;
      {
        Str8 str_before_right_bound = str8_substring(buffer_str, 0, right_bound + 1);
        width_before_right_bound = ui_measure_text(str_before_right_bound).x;
      }
      F32 width_inbound_str = 0.0f;
      {
        Str8 inbound_str = str8_substring(buffer_str, left_bound, right_bound + 1);
        width_inbound_str = ui_measure_text(inbound_str).x;
      }
      Assert(width_before_right_bound >= width_inbound_str);

      F32 extra_space = 0.0f;
      if (left_bound > 0) // This is made so we offset correctly and partially see char that is not fully visible to the left of the left bound
      {
        extra_space = edit_box_clip_data.on_screen_dims.x - width_inbound_str;  
      }
      F32* text_clip_x = &edit_box_clip_data.clip_offset->x;
      *text_clip_x = -(width_before_right_bound - width_inbound_str - extra_space); 
    }

    Assert(left_bound <= cursor_pos && cursor_pos <= (right_bound + 1));
  }

  // Per frame cusor clip offset
  F32 cursor_clip_offset = 0.0f;
  {
    Str8 str_before_cursor = str8_substring(buffer_str, 0, cursor_pos);
    F32 size_before_cursor = ui_measure_text(str_before_cursor).x;
    cursor_clip_offset = size_before_cursor - (-1.0f * edit_box_clip_data.clip_offset->x);
  }
  
  // Per frame section clip offset
  F32 section_clip_offset = 0.0f;
  {
    Str8 str_before_section = str8_substring(buffer_str, 0, section_start_pos);
    F32 size_before_section = ui_measure_text(str_before_section).x;
    section_clip_offset = size_before_section - (-1.0f * edit_box_clip_data.clip_offset->x);
  }

  edit_box_state->left_bound = left_bound;
  edit_box_state->right_bound = right_bound;

  Font font       = ui_get_font();      
  F32 font_size   = ui_get_font_size(); 
  F32 line_height = ui_measure_text_ex(Str8FromC("a"), font, font_size).y; // todo: Use a better way to retrive line height
  
  // UI routine
  B32 is_text_editing_done = false;

  Vec4_F32 text_color = {255, 255, 255, 255};
  ui_push_font(font);
  ui_push_font_size(font_size);
  ui_push_text_color(text_color);
  {  
    ui_set_next_size_x(edit_box_size_x);
    ui_set_next_size_y(ui_px(line_height));
    ui_set_next_layout_axis(Axis2__x);
    ui_add_flags_to_next(UI_Box_flag__dont_draw_overflow);
    UI_Box* edit_box = ui_box_make(edit_box_id, 0);
    
    UI_Actions edit_box_actions = ui_actions_from_box(edit_box, rli_events);
    if (!ui_is_active(edit_box_id) && edit_box_actions.is_clicked)
    {
      ui_set_active(edit_box_id);
    }

    UI_Parent(edit_box)
    {
      if (!ui_is_active(edit_box_id) && buffer_str.count != 0)
      {
        ui_label(buffer_str);
      }
      else if (!ui_is_active(edit_box_id) && buffer_str.count == 0)
      {
        ui_label(placeholder_str);
      }
      else if (ui_is_active(edit_box_id) && cursor_pos == section_start_pos)
      {
        ui_label(buffer_str);
        
        // Cursor
        ui_set_next_size_x(ui_p_of_p(1.0f, 1.0f)); 
        ui_set_next_size_y(ui_px(line_height));
        ui_set_next_layout_axis(Axis2__x);
        ui_set_next_flags(UI_Box_flag__floating); 
        UI_Box* cursor_clip_box = ui_box_make(Str8{}, 0);
        {
          if (cursor_clip_offset >= cursor_width) { 
            cursor_clip_box->clip_offset.x = cursor_clip_offset - cursor_width; // todo: This is not the best. This makes it so if the cursor is at the end of the visible string it doesnt not disappear due to clipping
          }
        }
        UI_Parent(cursor_clip_box)
        {
          ui_set_next_size_x(ui_px(cursor_width));
          ui_set_next_size_y(ui_px(line_height));
          ui_set_next_color({ 0, 0, 255, 255 });
          ui_set_next_flags(UI_Box_flag__draw_background);
          UI_Box* cursor_box = ui_box_make(Str8{}, 0);
        }
      }
      /*
      else if (ui_is_active(edit_box_id) && cursor_pos != section_start_pos)
      {
        Str8 str_before_section = str8_substring(buffer_str, 0, Min(section_start_pos, cursor_pos));
        ui_label(str_before_section);

        Str8 str_section = str8_substring(buffer_str, Min(section_start_pos, cursor_pos), Max(section_start_pos, cursor_pos));
        F32 str_section_size = ui_measure_text(str_section).x;
        ui_set_next_size_x(ui_px(str_section_size));
        ui_set_next_size_y(ui_px(line_height));
        (void)ui_box_make(Str8{}, 0);
        
        Str8 str_after_section = str8_substring(buffer_str, Max(section_start_pos, cursor_pos), buffer_str.count);
        ui_label(str_after_section);
        
        ui_set_next_size_x(ui_p_of_p(1.0f, 1.0f)); 
        ui_set_next_size_y(ui_px(line_height));
        ui_set_next_layout_axis(Axis2__x);
        ui_set_next_flags(UI_Box_flag__floating); 
        UI_Box* section_clip_box = ui_box_make(Str8{}, 0);
        {
          F32* section_clip = &section_clip_box->clip_offset.x;
          *section_clip = Min(section_clip_offset, cursor_clip_offset);
        }
        UI_Parent(section_clip_box)
        {
          ui_set_next_flags(UI_Box_flag__draw_background);
          ui_set_next_color({ 0, 0, 255, 255 });
          ui_label(str_section);
        }
        
      }
      else {
        InvalidCodePath();
      }
      */
    }
  }

  return is_text_editing_done;
}

/*
B32 ui_text_box_mutates(Str8 edit_box_id, Str8 placeholder_str, UI_Text_edit_box_state* edit_box_state, UI_Size edit_box_size_x, U8* text_buffer, U64 buffer_max_size, U64* buffer_current_size, U64* cursor_pos, U64* section_start_pos, RLI_Event_list* rli_events)
{
  Scratch scratch = get_scratch(0, 0);
  UI_Text_op_list text_op_list = ui_text_op_list_from_rli_events(scratch.arena, rli_events);
  ui_aply_text_ops(text_op_list, text_buffer, buffer_max_size, buffer_current_size, cursor_pos, section_start_pos);
  B32 enter_got_pressed = ui_text_edit_box_static(edit_box_id, placeholder_str, edit_box_state, edit_box_size_x, text_buffer, buffer_max_size, *buffer_current_size, *cursor_pos, *section_start_pos, rli_events);
  end_scratch(&scratch);
  return enter_got_pressed;
}
*/

#endif