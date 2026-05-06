#ifndef __UI_WIDGETS_H
#define __UI_WIDGETS_H

#include "ui/ui_core.h"

// - Simple widgets
// void ui_label_c(const char* c_str);
// void ui_label(Str8 str);
// void ui_label_fmt(const char* fmt, ...);
// void ui_spacer(UI_Size size);
// UI_Actions ui_button(Str8 str, RLI_Event_list* rli_events); // todo: Remove the fucking rli events from there dude

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
  // F32 corner_r;
  const char* fmt_str;
  
  V4 hover_color;
  V4 no_hover_color;
  V4 slided_part_color;
  
  V4 text_color;
  F32 font_size;
};
F32 ui_slider(Str8 slider_id, const UI_Slider_style* slider_style, F32 current_value, F32 min, F32 max)
{
  UI_Actions slider_actions = ui_actions_from_id(slider_id);

  if (slider_actions.is_hovered) {
    ui_set_next_b_color(slider_style->hover_color);
  } else {
    ui_set_next_b_color(slider_style->no_hover_color);
  }
  ui_set_next_size_x(ui_px(slider_style->width));
  ui_set_next_size_y(ui_px(slider_style->height));
  // ui_set_next_corner_radius(slider_style->corner_r);
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* slider_box = ui_box_make(slider_id, UI_Box_flag__dont_draw_overflow);

  B32 is_data_present = true;

  Rect slider_box_rect = {};
  {
    UI_Box_data slider_box_data = ui_get_box_data_prev_frame(slider_id);
    is_data_present &= slider_box_data.is_found;
    slider_box_rect = slider_box_data.on_screen_rect;
  }

  if (slider_actions.is_hovered)
  {
    current_value += slider_actions.wheel_move;
    clamp_f32_inplace(&current_value, min, max);
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
      ui_set_next_font_size(slider_style->font_size);
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

// // - Text input field 
// U64 __ui_move_with_control_left(Str8 str, U64 current_pos); 
// U64 __ui_move_with_control_right(Str8 str, U64 current_pos);

// enum UI_Text_op_kind {  
//   UI_Text_op_kind__NONE,
//   UI_Text_op_kind__move_cursor,
//   UI_Text_op_kind__delete_section,
//   UI_Text_op_kind__copy_section,
//   UI_Text_op_kind__paste_at_cursor,
//   UI_Text_op_kind__insert_char_at_cursor, 
// };

// enum UI_Text_op_move_specifier {
//   UI_Text_op_move_specifier___NONE,
//   UI_Text_op_move_specifier___move_1_char_left,                     
//   UI_Text_op_move_specifier___move_1_char_right,                    
//   UI_Text_op_move_specifier___move_1_word_left,                     
//   UI_Text_op_move_specifier___move_1_word_right,                    
//   UI_Text_op_move_specifier___move_to_line_start,                   
//   UI_Text_op_move_specifier___move_to_line_end,                     
// };

// struct UI_Text_op {
//   UI_Text_op_kind kind;

//   // Fat struct data
//   U8 char_to_insert;               
//   B8 keep_section_start_after_op; 
//   B8 dont_move_if_section;
//   B8 override_move_and_move_to_section_min_if_ending_section;
//   B8 override_move_and_move_to_section_max_if_ending_section;
//   UI_Text_op_move_specifier move_specifier;

//   UI_Text_op* next;
//   UI_Text_op* prev;
// };

// struct UI_Text_op_list {
//   UI_Text_op* first;
//   UI_Text_op* last;
//   U64 count;
// };

// UI_Text_op* ui_text_op_list_push(Arena* arena, UI_Text_op_list* list, UI_Text_op_kind kind);
// UI_Text_op_list ui_text_op_list_from_rli_events(Arena* arena, RLI_Event_list* event_list);
// void ui_aply_text_ops(UI_Text_op_list text_op_list, U8* text_buffer, U64 max_text_size, U64* current_text_size, U64* cursor_pos, U64* section_start);

// ///////////////////////////////////////////////////////////
// // - Attempt to redo text edit box again (this shoud like like the text edit box for the search bar on youtube)
// //
// struct UI_Text_edit_box_style {
//   F32 width_in_px;
//   Font font;
//   F32 font_size;
//   // V4 text_color;
//   // add: value that specifies how many simbols are minimum on a side to start changing clip offset
// };
// struct UI_Text_edit_box_state {
//   // F32 non_used_space_right;

//   // F32 walled_part_space;

//   // F32 visible_extra_space_left;
//   // F32 visible_extra_space_right;

//   // U64 wall_index_left;  // These indexes are kind of like cursor, so the range is [left, right) to get he walled string
//   // U64 wall_index_right;
// };
// void ui_text_edit_box(
//   UI_Text_edit_box_state* state,
//   const UI_Text_edit_box_style* edit_box_style, 
//   U8* text_buffer, 
//   U64 text_buffer_max_size, // todo: I feel like for displaying this is not really needed 
//   U64 text_buffer_current_size, 
//   U64 cursor, 
//   U64 section,
//   RLI_Event_list* rli_events
//   // U64* left_wall_index,
//   // U64* right_wall_index 
// ) {
//   /* fix:
//     - i want you to understand the code for the ui widget completely
//   */

//   /* add:
//     - cursor size setting
//   */
  
//   /* idea:
//     - split the edit box space into 3 things, "extra on the left", "extra on the right", "walled in the middle".
//       All these sizes are stored in retained state, which allows then for easy checking for change.
//       We then might know if something was added or removed.
//       The left and right extra spaces might be 0 sized and have to be the same between updates.
//       This way we only update them in the update loop and check the walled part since that is the only
//       part in which addition was possible.ddfdklfjsdklfjsdkl;jfkl;asjf 

//     - you might just make 2 different boxes and the cursor box will draw ouside the box.
//       This will allow the caller if they specify a large cursor to just pad the box up and have the 
//       cursor still be on the next character and not the prev
//   */

//   Str8 text_buffer_as_str = str8_manuall(text_buffer, text_buffer_current_size);

//   // ---- Testing this 
//   static F32 non_used_space_right = 0.0f;

//   // ---- Testing this 
//   static F32 cursor_offset_from_left_wall = 0.0f;

//   // State
//   static F32 walled_part_space = 0.0f;
//   // 
//   static F32 visible_extra_space_left = 0.0f;
//   static F32 visible_extra_space_right = 0.0f;
//   // 
//   static U64 wall_index_left = 0;  // These indexes are kind of like cursor, so the range is [left, right) to get he walled string
//   static U64 wall_index_right = 0;

//   // Updating state
//   if (cursor > wall_index_right)
//   {
//     // When we move over the right wall, we make the new position be the right wall.
//     // The right extra space shoud also become 0, since now start rendering px perfect char end and the 
//     // cursor on it. On the left side there might be additional part of char that cant fit into the walled 
//     // part but is still visible. That is represented by the additional space left. Additional space right
//     // has to be 0. 
//     // 
//     // todo: What about when we have text that is smaller than the box, what about that space ???

//     wall_index_right = cursor;
//     visible_extra_space_right = 0.0f;  
    
//     // This shoud be true, since thats the only way that it is smaller than cursor, to which we then set it
//     Assert(wall_index_right > 0); 

//     // Finding the index for the new left wall
//     F32 accumulated_text_width = 0.0f;
//     for (U64 test_left_wall_index = wall_index_right - 1;;)
//     {
//       // Since we know that the right side has no extra visible space, we just accumulate untill we can
//       Str8 test_char_str = str8_substring(text_buffer_as_str, test_left_wall_index, test_left_wall_index + 1);
//       V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
//       accumulated_text_width += dims.x;
//       if (accumulated_text_width > edit_box_style->width_in_px)
//       {
//         wall_index_left = test_left_wall_index + 1; // We over reached, so + 1
//         accumulated_text_width -= dims.x;
//         break;
//       }
//       if (test_left_wall_index == 0) { break; }
//       test_left_wall_index -= 1;
//     }

//     Assert(visible_extra_space_right == 0.0f); // Just for context
//     // if (visible_extra_space_left == 0.0f && accumulated_text_width < edit_box_style->width_in_px)
//     if (wall_index_left == 0 && accumulated_text_width < edit_box_style->width_in_px)
//     {
//       // Walled strings often are not the same lengh than edit_box_style->width_in_px.
//       // They also cant be longer, that would be invalid state.
//       // But if the left wall index is 0 and the walled part is less than the space available in the 
//       // edit box, that means that out string is not long enough to be longer than the edit box space.
//       // If it was we would eihter have accumulated_text_width be the same as the edit box size,
//       // or would have some space on the left. But we dont have it, since the left wall is 0;
//       Assert(visible_extra_space_left == 0.0f);
//       // note: I dont use visible_extra_space_left in the condition, since when state is first passed,
//       // all the values there are 0. Wall index left at 0 is valid. If i used visible_extra_space_left 
//       // in the condition, if would always then trigger this code path and never set visible_extra_space_left
//       // to a different value in the else block below.

//       non_used_space_right = edit_box_style->width_in_px - accumulated_text_width;
//     }
//     else
//     {
//       visible_extra_space_left = edit_box_style->width_in_px - accumulated_text_width;
//       non_used_space_right = 0.0f;
//       Assert(visible_extra_space_left >= 0.0f);
//     }
//   }
//   else if (cursor < wall_index_left)
//   {
//     wall_index_left = cursor;
//     visible_extra_space_left = 0.0f;  

//     F32 accumulated_text_width = 0.0f;
//     for (U64 test_right_wall_index = wall_index_left;;)
//     { 
//       // Since we know that left side has 0 extra visible space, we just accumulate until we cant
//       Str8 test_char_str = str8_substring(text_buffer_as_str, test_right_wall_index, test_right_wall_index + 1);
//       V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
//       accumulated_text_width += dims.x;
//       if (accumulated_text_width > edit_box_style->width_in_px)
//       {
//         accumulated_text_width -= dims.x;
//         wall_index_right = test_right_wall_index; // Since wall indexes work the same as cursor, they are at the next index kind of, we dont do -1 or +1 here
//         break;
//       }
//       if (test_right_wall_index == text_buffer_max_size) { break; }
//       test_right_wall_index += 1;
//     }

//     visible_extra_space_right = edit_box_style->width_in_px - accumulated_text_width;
    
//     // We shoud not have any non used space on the right of the text edit box, 
//     // since if we have moved to the left of the cursor, that means that 
//     // the text at some point was longer than the edit box size, which means 
//     // that its not possible to have non used space when we move left
//     non_used_space_right = edit_box_style->width_in_px - (visible_extra_space_left + accumulated_text_width + visible_extra_space_right);
//     Assert(non_used_space_right == 0.0f);
//   }
//   else if (wall_index_left == 0 && wall_index_right == 0) { 
//     // Nothing, waiting for the user to do something to the cursor 
//     // to trigger the routine above
//   }
//   else 
//   {
//     Str8 walled_str = str8_substring(text_buffer_as_str, wall_index_left, wall_index_right);
//     V2 walled_dims = ui_measure_text_ex(walled_str, edit_box_style->font, edit_box_style->font_size);
    
//     if (walled_dims.x > walled_part_space) // Something got inserted (this did not happend at the right wall, since if it did, we would have handled it in the case where cursor > right_wall_index)
//     {
//       BreakPoint();
//       Assert(wall_index_right > 0);

//       // Since something got added, we know that the char that got added is not the same size as the one 
//       // that was at the right wall boundary. If it was, then there would be no difference, since when we add
//       // we push the right char to the right. So we for example 'r' was added and another 'r' was on the right
//       // wall boundary, the one on the boundary would get pushed outside the boundary and the new one would be
//       // added in the middle of the walled part. Either way to us it makes no difference in how to display it,
//       // since nothing changed in terms of the positioning and spacing.

//       if (visible_extra_space_left + walled_dims.x + visible_extra_space_right > edit_box_style->width_in_px)
//       {
//         // When adding a char, we relocate the right wall, that means that the right visible extra space
//         // might also shift, so we dont care about it here, since its not stable, unlike the extra 
//         // space on the left
//         F32 space_to_remove = (visible_extra_space_left + walled_dims.x + visible_extra_space_right) - edit_box_style->width_in_px;
//         space_to_remove -= visible_extra_space_right;
//         if (space_to_remove < 0.0f) // Only some extra space shoud be removed 
//         {
//           BreakPoint();
//           visible_extra_space_right = abs_f32(space_to_remove);
//         }
//         if (space_to_remove > 0.0f) // Now we also have to adjust the right wall
//         {
//           F32 total_space_removed = 0.0f;
//           U64 test_right_index = wall_index_right - 1;
//           for (;;)
//           {
//             Str8 test_char_str = str8_substring(text_buffer_as_str, test_right_index, test_right_index + 1);
//             V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
//             total_space_removed += dims.x;
//             if (total_space_removed > space_to_remove)
//             {
//               visible_extra_space_right = total_space_removed - space_to_remove; 
//               break;
//             }
//             if (test_right_index == 0) { break; } // todo: It cant really reach 0 unless it also reached the left wall, give more thought to this edge case
//             test_right_index -= 1;
//           }
//           wall_index_right = test_right_index;
//         }
//       }
//     }
//     else 
//     if (walled_dims.x < walled_part_space) // Something got deleted (this did not happend at the left wall, since if it did, we would have had the routine (cursor < left_wall_index) above handle it)
//     {
//       Assert(wall_index_right > 0);
//       BreakPoint();
      
//       // goal: 
//       // We want to keep the cursor at the same offset to the user if possible.
//       // So when we are removing chars, we want the cursor to be in the same spot as it was before.
//       // We need to know what cursor offset to the left wall was. We know the left extra space that the user sees,
//       // now we just need to know how much till the cursor they also seen      
      
      
//       // === Old code
//       /*
//       F32 space_to_add = walled_part_space - walled_dims.x + visible_extra_space_right;
//       F32 total_space_added = 0.0f;
//       U64 test_right_index = wall_index_right;
//       for (;;)
//       {
//         if (test_right_index >= text_buffer_as_str.count) { break; } // This is in case if we were at the end and the last char got removed
//         Str8 test_char_str = str8_substring(text_buffer_as_str, test_right_index, test_right_index + 1);
//         V2 dims = ui_measure_text_ex(test_char_str, edit_box_style->font, edit_box_style->font_size);
//         total_space_added += dims.x;
//         if (total_space_added > space_to_add)
//         {
//           total_space_added -= dims.x;
//           break;
//         }
//         test_right_index += 1;
//       }

//       // todo: I dont understand this
//       visible_extra_space_right = space_to_add - total_space_added;  
//       wall_index_right = test_right_index;
//       */
//     }
//   }

//   // Updating wrapped part size 
//   {
//     Str8 walled_str = str8_substring(text_buffer_as_str, wall_index_left, wall_index_right);
//     V2 dims = ui_measure_text_ex(walled_str, edit_box_style->font, edit_box_style->font_size);
//     walled_part_space = dims.x;
//   }
  
//   // Updating cursor_offset_from_left_wall
//   {
//     Str8 walled_cursor_str = str8_substring(text_buffer_as_str, wall_index_left, cursor);
//     V2 dims = ui_measure_text_ex(walled_cursor_str, edit_box_style->font, edit_box_style->font_size);
//     cursor_offset_from_left_wall = dims.x;
//   }

//   // Just checking if the invariant has not been broken
//   if (
//     wall_index_right != 0 // This means that the state is not zeroed out and not yet set by the routine
//       && 
//     (visible_extra_space_left + walled_part_space + visible_extra_space_right + non_used_space_right != edit_box_style->width_in_px)
//   ) {
//     BreakPoint();
//   }

//   F32 height_in_px = 0.0f;
//   {
//     V2 dims = ui_measure_text_ex(Str8FromC(" "), edit_box_style->font, edit_box_style->font_size);
//     height_in_px = dims.y;
//   }

//   Str8 edit_box_id = Str8FromC("Edit box id");
//   ui_set_next_size_x(ui_px(edit_box_style->width_in_px));
//   ui_set_next_size_y(ui_px(height_in_px));
//   UI_Box* edit_box = ui_box_make(edit_box_id, UI_Box_flag__dont_draw_overflow);
  
//   F32 edit_box_clip_offset = 0.0f;
//   {
//     if (visible_extra_space_left != 0.0f && visible_extra_space_right == 0.0f)
//     {
//       Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, wall_index_left);
//       V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//       edit_box_clip_offset = dims.x - visible_extra_space_left;
//     }
//     else if (visible_extra_space_left == 0.0f && visible_extra_space_right != 0.0f)
//     {
//       Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, wall_index_left);
//       V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//       edit_box_clip_offset = dims.x;
//     }
//     else if (visible_extra_space_left != 0.0f && visible_extra_space_right != 0.0f)
//     {
//       Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, wall_index_left);
//       V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//       edit_box_clip_offset = dims.x - visible_extra_space_left;
//     }
//     else if (visible_extra_space_left == 0.0f && visible_extra_space_right == 0.0f)
//     {
//       Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, wall_index_left);
//       V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//       edit_box_clip_offset = dims.x;
//     }
//     else { InvalidCodePath(); }
//   }
//   edit_box->clip_offset.x = -1 * edit_box_clip_offset;

//   /*note:
//     The parent box. This is the box that is for the user, call it the text edit box.
//     It has all the rest of the things needed to draw the text edit inside of it. 
//     Other things that it need are:
//       - Cursor. 
//       - Section
//     Both of these are child boxes but positioned absolutely. 
//     Each of these boxed aplies its own clip_offset. This is easier since this way there is a single truth, the text size.
//     All the cursors and the text will just be offset relative to the text.
//     Then it is the job of the implementor to make sure that cursor is never outside of the screen and 
//     looks weird or gets clipped and then its like there is no cursor to the user.   
//   */

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
//     cursor_offset_box->clip_offset.x = -1 * edit_box_clip_offset;

//     // Getting size for the spacer that will offset the cursor relative the the edit box start pos
//     F32 cursor_offset_spacer = 0.0f;
//     if (visible_extra_space_right == 0 && cursor == wall_index_right)
//     {
//       Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, wall_index_right);
//       V2 dims = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
//       cursor_offset_spacer = dims.x - cursor_width;
//     }
//     else
//     {
//       Str8 str_till_cursor = str8_substring(text_buffer_as_str, 0, cursor);
//       V2 dims = ui_measure_text_ex(str_till_cursor, edit_box_style->font, edit_box_style->font_size);
//       cursor_offset_spacer = dims.x;
//     }

//     UI_Parent(cursor_offset_box)
//     {
//       ui_spacer(ui_px(cursor_offset_spacer));

//       ui_set_next_size_x(ui_px(cursor_width)); ui_set_next_size_y(ui_px(height_in_px));
//       ui_set_next_color({ 255, 255, 255, 255 });
//       UI_Box* cursor_line_box = ui_box_make(Str8{}, 0);
//     }

//     #define DRAW_DEBUG_WALLS 1
//     #if DRAW_DEBUG_WALLS
//     {
//       ui_set_next_size_x(ui_px(2)); ui_set_next_size_y(ui_px(height_in_px));
//       ui_set_next_layout_axis(Axis2__x);
//       ui_set_next_flags(UI_Box_flag__floating);
//       UI_Box* left_wall_offset_box = ui_box_make(Str8{}, 0);
//       left_wall_offset_box->clip_offset.x = -1 * edit_box_clip_offset;
      
//       UI_Parent(left_wall_offset_box)
//       {
//         Str8 str_till_left_wall = str8_substring(text_buffer_as_str, 0, wall_index_left);
//         V2 dims = ui_measure_text_ex(str_till_left_wall, edit_box_style->font, edit_box_style->font_size);
//         F32 offset = dims.x;
//         ui_spacer(ui_px(offset));

//         ui_set_next_size_x(ui_px(2)); ui_set_next_size_y(ui_px(height_in_px));
//         ui_set_next_color({ 0, 255, 255, 255 });
//         UI_Box* left_wall = ui_box_make(Str8{}, 0);
//       }
//     }
//     #endif
//     #undef DRAW_DEBUG_WALLS

//     #define DRAW_DEBUG_WALLS 1
//     #if DRAW_DEBUG_WALLS
//     {
//       ui_set_next_size_x(ui_px(2)); ui_set_next_size_y(ui_px(height_in_px));
//       ui_set_next_layout_axis(Axis2__x);
//       ui_set_next_flags(UI_Box_flag__floating);
//       UI_Box* left_wall_offset_box = ui_box_make(Str8{}, 0);
//       left_wall_offset_box->clip_offset.x = -1 * edit_box_clip_offset;
      
//       UI_Parent(left_wall_offset_box)
//       {
//         Str8 str_till_right_wall = str8_substring(text_buffer_as_str, 0, wall_index_right);
//         V2 dims = ui_measure_text_ex(str_till_right_wall, edit_box_style->font, edit_box_style->font_size);
//         ui_spacer(ui_px(dims.x - 1));

//         ui_set_next_size_x(ui_px(2)); ui_set_next_size_y(ui_px(height_in_px));
//         ui_set_next_color({ 255, 0, 255, 255 });
//         UI_Box* right_wall = ui_box_make(Str8{}, 0);
//       }

//     }
//     #endif
//     #undef DRAW_DEBUG_WALLS
//   }
// }



#endif