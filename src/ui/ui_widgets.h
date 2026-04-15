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
struct UI_Text_edit_box_style {
  F32 width_in_px;
  Font font;
  F32 font_size;
  // V4 text_color;
  // add: value that specifies how many simbols are minimum on a side to start changing clip offset
};
void ui_text_edit_box(
  const UI_Text_edit_box_style* edit_box_style, 
  U8* text_buffer, 
  U64 text_buffer_max_size, // todo: I feel like for displaying this is not really needed 
  U64 text_buffer_current_size, 
  U64 cursor, 
  U64 section,
  RLI_Event_list* rli_events,
  U64* left_wall_index,
  U64* right_wall_index 
) {
  /* fix:
    - cursor is not visible when at 0
    - i want you to understand the code for the ui widget completely
  */

  /* add:
    - cursor size setting
  */
  
  Str8 text_buffer_as_str = str8_manuall(text_buffer, text_buffer_current_size);

  // Check if the right wall is still valid. 
  // Redo the right wall if it is not and keep the edit box offset
  // Check if walls are valid relative the the cursor pos
  // If they are then great
  // If they are not then we move them and reset the edit box offset 
  // Then just draw the ui

  // Checking if the right wall is still valid
  {
    Str8 walled_str = str8_substring(text_buffer_as_str, *left_wall_index, *right_wall_index);
    V2 walled_dims = ui_measure_text_ex(walled_str, edit_box_style->font, edit_box_style->font_size);
    F32 walled_str_width = walled_dims.x;

    if (walled_str_width > edit_box_style->width_in_px)
    {
      // Move the wall back till its valid
      F32 space_to_remove = walled_dims.x - edit_box_style->width_in_px;
      if (*right_wall_index > 0)
      {
        F32 total_space_removed = 0.0f;
        for (U64 test_right_wall_index = *right_wall_index - 1 ;; test_right_wall_index -= 1)
        {
          Str8 test_char_to_remove = str8_substring(text_buffer_as_str, test_right_wall_index, test_right_wall_index + 1);
          V2 test_char_dims = ui_measure_text_ex(test_char_to_remove, edit_box_style->font, edit_box_style->font_size);
          total_space_removed += test_char_dims.x;
          if (total_space_removed > space_to_remove)
          {
            *right_wall_index = test_right_wall_index;
            break;
          }
          if (test_right_wall_index == 0) { break; }
        }
      }
    }
    else if (walled_str_width < edit_box_style->width_in_px)
    {
      // Move the wall front till its valid
      F32 space_to_add = edit_box_style->width_in_px - walled_dims.x;
      if (*right_wall_index < text_buffer_as_str.count)
      {
        F32 total_space_added = 0.0f;
        for (U64 test_right_wall_index = *right_wall_index ;; test_right_wall_index += 1)
        {
          Str8 test_char_to_add = str8_substring(text_buffer_as_str, test_right_wall_index, test_right_wall_index + 1);
          V2 test_char_dims = ui_measure_text_ex(test_char_to_add, edit_box_style->font, edit_box_style->font_size);
          total_space_added += test_char_dims.x;

          if (total_space_added > space_to_add)
          {
            *right_wall_index = test_right_wall_index - 1;
            break;
          }
          if (test_right_wall_index == text_buffer_as_str.count) { break; }
        }
      }
    }
  }

  F32 height_in_px = 0.0f;
  {
    V2 dims = ui_measure_text_ex(Str8FromC(" "), edit_box_style->font, edit_box_style->font_size);
    height_in_px = dims.y;
  }

  static B32 is_left_wall_main = false;
  static B32 is_right_wall_main = false;
  static B32 edit_box_clip_offset = 0.0f;

  // Updating walls if have to
  if (cursor > *right_wall_index) 
  {
    *right_wall_index = cursor;
    
    F32 accumulated_text_width = 0.0f;
    for (U64 test_left_wall_index = *right_wall_index - 1;; )
    {
      Str8 test_char_str = str8_substring(text_buffer_as_str, test_left_wall_index, test_left_wall_index + 1);
      V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
      accumulated_text_width += dims.x;
      if (accumulated_text_width > edit_box_style->width_in_px)
      {
        *left_wall_index = test_left_wall_index + 1;
        break;
      }
      if (test_left_wall_index == 0) { break; }
      test_left_wall_index -= 1;
    }

    is_right_wall_main = true;
    is_left_wall_main = false;
  }
  else if (cursor < *left_wall_index)
  {
    *left_wall_index = cursor;

    F32 accumulated_text_width = 0.0f;
    for (U64 test_right_wall_index = *left_wall_index;;)
    { 
      Str8 test_char_str = str8_substring(text_buffer_as_str, test_right_wall_index, test_right_wall_index + 1);
      V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
      accumulated_text_width += dims.x;
      if (accumulated_text_width > edit_box_style->width_in_px)
      {
        *right_wall_index = test_right_wall_index - 1;
        break;
      }
      if (test_right_wall_index == text_buffer_max_size) { break; }
      test_right_wall_index += 1;
    }

    is_left_wall_main = true;
    is_right_wall_main = false;
  }

  // Just checking stuff 
  {
    Str8 walled_str = str8_substring(text_buffer_as_str, *left_wall_index, *right_wall_index);
    V2 walled_dims = ui_measure_text_ex(walled_str, edit_box_style->font, edit_box_style->font_size);
    if (walled_dims.x > edit_box_style->width_in_px)
    {
      Assert(false);
    }
  }

  Str8 edit_box_id = Str8FromC("Edit box id");
  ui_set_next_color({ 255, 255, 255, 255 });
  ui_set_next_size_x(ui_px(edit_box_style->width_in_px));
  ui_set_next_size_y(ui_px(height_in_px));
  ui_set_next_border({1, { 255, 0, 0, 255 }});
  UI_Box* edit_box = ui_box_make(edit_box_id, UI_Box_flag__dont_draw_overflow);

  // Updating global ui activeness 
  static B32 is_active = false;
  UI_Actions edit_box_actions = ui_actions_from_box(edit_box, rli_events);
  if (edit_box_actions.is_clicked && !is_active)
  {
    is_active = true; 
  }

  // ==============================================
  // note: This might be removed, since this thing might also be done only when the wall is set up,
  //       since every other time it will be the same until the walls change 
  //       and there is a new main wall. That might be done when the walls are set.
  // if the right wall is main, then we have to make the offset be the offset of string inside the wall and the extras space
  F32 clip_offset = 0.0f;
  if (is_right_wall_main)
  {
    Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, *right_wall_index + 1);
    V2 till_right_wall_dims  = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
    
    clip_offset = till_right_wall_dims.x - edit_box_style->width_in_px;
    if (clip_offset < 0.0f) { clip_offset = 0.0f; }
  }
  else if (is_left_wall_main)
  { 
    Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, *left_wall_index);
    V2 till_left_wall_dims  = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
    clip_offset = till_left_wall_dims.x;
  }
  // ==============================================


  /*note:
    The parent box. This is the box that is for the user, call it the text edit box.
    It has all the rest of the things needed to draw the text edit inside of it. 
    Other things that it need are:
      - Cursor. 
      - Section
    Both of these are child boxes but positioned absolutely. 
    Each of these boxed aplies its own clip_offset. This is easier since this way there is a single truth, the text size.
    All the cursors and the text will just be offset relative to the text.
    Then it is the job of the implementor to make sure that cursor is never outside of the screen and 
    looks weird or gets clipped and then its like there is no cursor to the user.   
  */

  edit_box->clip_offset.x = -1 * clip_offset;
  UI_Parent(edit_box)
  {
    // Text 
    ui_label(text_buffer_as_str);
  
    // Cursor offset box
    F32 cursor_width = 5.0f;
    
    ui_set_next_size_x(ui_px(edit_box_style->width_in_px)); ui_set_next_size_y(ui_px(height_in_px));
    ui_set_next_layout_axis(Axis2__x);
    ui_set_next_flags(UI_Box_flag__floating);
    UI_Box* cursor_offset_box = ui_box_make(Str8{}, 0);

    F32 cursor_box_clip_offset = clip_offset; // clip_offset is for the edit box
    cursor_offset_box->clip_offset.x = -1 * cursor_box_clip_offset;

    // =====================================
    // note: This might be removed, since this is just cursor placement adjustment based on 
    //       first and last index to make it not overflow
    F32 cursor_offset_spacer = 0.0f;
    if (is_left_wall_main && cursor == *left_wall_index)
    {
      Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, *left_wall_index);
      V2 till_left_wall_dims  = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
      cursor_offset_spacer = till_left_wall_dims.x;
    }
    else if (is_right_wall_main && cursor == *right_wall_index)
    {
      Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, *right_wall_index);
      V2 till_right_wall_dims  = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
      cursor_offset_spacer = till_right_wall_dims.x - cursor_width;
    }
    else // We are in the middle of the walls 
    {
      Str8 str_till_cursor = str8_substring(text_buffer_as_str, 0, cursor);
      V2 till_cursor_dims  = ui_measure_text_ex(str_till_cursor, edit_box_style->font, edit_box_style->font_size);
      cursor_offset_spacer = till_cursor_dims.x;
    }
    // =====================================

    UI_Parent(cursor_offset_box)
    {
      ui_spacer(ui_px(cursor_offset_spacer));

      ui_set_next_size_x(ui_px(cursor_width)); ui_set_next_size_y(ui_px(height_in_px));
      ui_set_next_color({ 0, 0, 0, 255 });
      UI_Box* cursor_line_box = ui_box_make(Str8{}, 0);
    }

    #define DRAW_DEBUG_WALLS 1
    #if DRAW_DEBUG_WALLS
    {
      ui_set_next_size_x(ui_px(3)); ui_set_next_size_y(ui_px(height_in_px));
      ui_set_next_layout_axis(Axis2__x);
      ui_set_next_flags(UI_Box_flag__floating);
      UI_Box* left_wall_offset_box = ui_box_make(Str8{}, 0);
      left_wall_offset_box->clip_offset.x = -1 * clip_offset;
      
      UI_Parent(left_wall_offset_box)
      {
        Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, *left_wall_index);
        V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
        F32 offset = dims.x;
        ui_spacer(ui_px(offset));

        ui_set_next_size_x(ui_px(3)); ui_set_next_size_y(ui_px(height_in_px));
        ui_set_next_color({ 0, 255, 0, 255 });
        UI_Box* left_wall = ui_box_make(Str8{}, 0);
      }

    }
    #endif
    #undef DRAW_DEBUG_WALLS

    #define DRAW_DEBUG_WALLS 1 
    #if DRAW_DEBUG_WALLS
    {
      ui_set_next_size_x(ui_px(3)); ui_set_next_size_y(ui_px(height_in_px));
      ui_set_next_layout_axis(Axis2__x);
      ui_set_next_flags(UI_Box_flag__floating);
      UI_Box* left_wall_offset_box = ui_box_make(Str8{}, 0);
      left_wall_offset_box->clip_offset.x = -1 * clip_offset;
      
      UI_Parent(left_wall_offset_box)
      {
        Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, *right_wall_index);
        V2 dims = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
        ui_spacer(ui_px(dims.x - 3));

        ui_set_next_size_x(ui_px(3)); ui_set_next_size_y(ui_px(height_in_px));
        ui_set_next_color({ 0, 0, 255, 255 });
        UI_Box* right_wall = ui_box_make(Str8{}, 0);
      }

    }
    #endif
    #undef DRAW_DEBUG_WALLS
  }
}



#endif