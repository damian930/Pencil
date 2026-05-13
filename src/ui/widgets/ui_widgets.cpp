#ifndef __UI_WIDGETS_CPP
#define __UI_WIDGETS_CPP

#include "ui/widgets/ui_widgets.h"

// ///////////////////////////////////////////////////////////
// // - Simple widgets
// //
// void ui_label_c(const char* c_str)
// {
//   U8* str_data = (U8*)const_cast<char*>(c_str);
//   Str8 str = str8_manuall(str_data, strlen(c_str));
//   ui_label(str);
// }

void ui_label(Str8 str)
{
  ui_set_next_size_x(ui_text_size()); 
  ui_set_next_size_y(ui_text_size());
  UI_Box* box = ui_box_make(str, UI_Box_flag__has_text_contents);
}

void ui_label_f(const char* fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  Scratch scratch = get_scratch(0, 0);
  U64 buffer_count = 128;
  U8* buffer = ArenaPushArr(scratch.arena, U8, buffer_count);
  int err = vsnprintf((char*)buffer, buffer_count, fmt, argptr);
  if (err < 0) { Assert(0); }
  else if (err >= buffer_count) { Assert(0); }
  else if (err < buffer_count) { /* All good */ }
  va_end(argptr);
  Str8 str = str8_manuall(buffer, (U64)err);
  ui_label(str);
  end_scratch(&scratch);
}

void ui_spacer(UI_Size size)
{  
  UI_Box* parent = ui_get_parent();
  if (parent->layout_axis == Axis2__x) { ui_set_next_size_x(size); ui_set_next_size_y(ui_px(0.0f)); }
  if (parent->layout_axis == Axis2__y) { ui_set_next_size_y(size); ui_set_next_size_x(ui_px(0.0f)); }
  ui_box_make(Str8{}, 0);
}

UI_Actions ui_button(Str8 str) // todo: Remove the fucking rli events from there dude
{
  ui_push_size_x(ui_text_size());
  ui_push_size_y(ui_text_size());
  UI_Box* box = ui_box_make(str, UI_Box_flag__has_background|UI_Box_flag__has_text_contents);
  UI_Actions actions = ui_actions_from_box(box);
  if (actions.is_down) { ui_set_active_box(box); }
  else if (!actions.is_down) { ui_reset_active_box_match(box); } // todo: This shoud also check if it is active, cause then if we just down on the button it will reset the global active state
  return actions;
}

///////////////////////////////////////////////////////////
// - Layout stacks
//
void ui_layout_stack_begin(Axis2 axis)
{
  // Str8 id = ui_get_next_id();
  Str8 id = {}; 
  ui_set_next_size_x(ui_children_sum());
  ui_set_next_size_y(ui_children_sum());
  ui_set_next_layout_axis(axis);
  UI_Box* stack = ui_box_make(id, 0); 
  ui_push_parent(stack);
}

void ui_layout_stack_end()
{
  ui_pop_parent();
}

///////////////////////////////////////////////////////////
// - Padded box
//
void ui_padded_box_begin(UI_Size size, Axis2 final_box_axis)
{
  // Str8 id = ui_get_next_id();
  ui_layout_stack_begin(axis2_other(final_box_axis));
  ui_spacer(size);
  ui_layout_stack_begin(final_box_axis);
  ui_spacer(size);
}

void ui_padded_box_end(UI_Size size)
{
  ui_spacer(size);
  ui_layout_stack_end();
  ui_spacer(size);
  ui_layout_stack_end();
}

///////////////////////////////////////////////////////////
// - Wrapper
//
// todo: How do we set the axis here, do we got the stack way or parameter way ???
void ui_wrapper_begin(Axis2 axis)
{
  // Str8 id = ui_get_next_id();
  Str8 id = {};
  ui_set_next_size_x(ui_children_sum());
  ui_set_next_size_y(ui_children_sum());
  ui_set_next_layout_axis(axis);
  UI_Box* wrapper = ui_box_make(id, 0);
  ui_push_parent(wrapper);
}

void ui_wrapper_end()
{
  ui_pop_parent();
}

///////////////////////////////////////////////////////////
// - Color pickers
//
struct _UI_Color_picker_sv_square_data {
  V4F32 colors[UV__COUNT];
};

void _ui_color_picker_sv_square_draw_func(UI_Box* box)
{
  _UI_Color_picker_sv_square_data* data = (_UI_Color_picker_sv_square_data*)box->custom_draw_data;
  d_add_rect_command_ex(box->final_on_screen_rect, data->colors[UV__00], data->colors[UV__01], data->colors[UV__10], data->colors[UV__11]);
}

void ui_color_picker_sv_square(Str8 id, UI_Size size_x, UI_Size size_y, V4F32 color_hsv, V4F32* out_opt_new_color_hsv)
{
  // 1) Do the ui based on the current state of the data
  //    - Draw the picker based on the provider prev color
  // 2) Based on the inputs, update the data and just give the new data to the user, keep the old one
  //    - Based on mouse pos + color picker dims, get the new color, return to the user
  
  // Setting up the color picke box
  ui_set_next_size_x(size_x);
  ui_set_next_size_y(size_y);
  UI_Box* color_picker_box = ui_box_make(id, 0);

  V4F32 pure_color_hsv = v4f32(color_hsv.hue, 1.0f, 1.0f, 1.0f);

  _UI_Color_picker_sv_square_data* draw_data = ArenaPush(ui_get_build_arena(), _UI_Color_picker_sv_square_data);
  draw_data->colors[UV__00] = white(); 
  draw_data->colors[UV__01] = black();
  draw_data->colors[UV__10] = rgb_from_hsv(pure_color_hsv);
  draw_data->colors[UV__11] = black();
  ui_box_set_custom_draw(color_picker_box, _ui_color_picker_sv_square_draw_func, draw_data);

  // note:
  // this picker is for sv, meaning for saturation and value, these are hsv values, not rgb
  // the value goes bottom-up in the color picker
  // the saturation goes left-right in the color picker
  // the bottom left and right are black
  // the top left is right
  // the top right is the purest version of the color. This is represented by hue, but in the rgb worls this
  // would have to be rgb_from_hsv(hue, 1.0f, 1.0f), so both value and saturation are 1.0s, this gives the most 
  // saturated and brigth color for a shade of color, which is specified by the hue, which is a 0->360* or
  // 0->1.0f value of the hsv color pallet. 

  UI_Box_data box_data = ui_get_box_data_prev_frame_from_box(color_picker_box);
  V2F32 mouse          = ui_get_mouse_pos();
  F32 circle_diameter  = 50.0f;

  F32 circle_x_offset = 0.0f;
  F32 circle_y_offset = 0.0f;
  if (box_data.found)
  {
    // Reverse lerp
    F32 x_t = color_hsv.saturation;
    F32 y_t = color_hsv.value ;
    clamp_f32_inplace(&x_t, 0.0f, 1.0f);
    clamp_f32_inplace(&y_t, 0.0f, 1.0f);

    circle_x_offset = (box_data.on_screen_rect.width * x_t) - (circle_diameter / 2.0f);
    circle_y_offset = (box_data.on_screen_rect.height * (1.0f - y_t)) - (circle_diameter / 2.0f);
  }

  // Color picker tree
  UI_Parent(color_picker_box)
  {
    UI_Col() 
    {
      ui_spacer(ui_px(circle_y_offset));
      UI_Row()
      {
        ui_spacer(ui_px(circle_x_offset));

        ui_set_next_b_color(white());
        ui_set_next_size_x(ui_px(circle_diameter));
        ui_set_next_size_y(ui_px(circle_diameter));
        UI_Box* circle_picker = ui_box_make({}, 0);
      }
    }
  }

  // Updating the colors 
  V4F32 new_color_hsv = color_hsv;
  UI_Actions actions = ui_actions_from_box(color_picker_box);
  if (!actions.is_down) { ui_reset_active_box_match(color_picker_box); }
  else {
    ui_set_active_box(color_picker_box); 
    
    if (box_data.found)
    {
      F32 picker_relative_x = (mouse.x - box_data.on_screen_rect.x) / (box_data.on_screen_rect.width);
      F32 picker_relative_y = 1.0f - ((mouse.y - box_data.on_screen_rect.y) / (box_data.on_screen_rect.height)); // Flipping the Y since color picker is bottom_left->up and the screen is top_left->down
      clamp_f32_inplace(&picker_relative_x, 0.0f, 1.0f);
      clamp_f32_inplace(&picker_relative_y, 0.0f, 1.0f);

      // Getting the color for the realative mouse pos
      new_color_hsv.saturation = picker_relative_x;
      new_color_hsv.value      = picker_relative_y;
    }
  }
  if (out_opt_new_color_hsv) { *out_opt_new_color_hsv = new_color_hsv; }
}

// ///////////////////////////////////////////////////////////
// // - Text input field
// //
// U64 __ui_move_with_control_left(Str8 str, U64 current_pos) 
// {
//   // note: CTRL+left moves to the start of a word.
//   //       If cursor is at the middle of a word, then we move to the start of the word the cursos is in the middle of.
//   //       If we are outside of a word, then we move to the start of the first word we find by moving left.
//   //       If we reach the start of a string, then we leave the cursor there.
//   if (current_pos > str.count)
//   {
//     Assert(current_pos <= str.count); // note: Some wrong with your parameter values dude. 
//     current_pos = str.count;
//   }
//   if (current_pos == 0) { return 0; }

//   B32 is_middle_of_word = char_is_word_char(str.data[current_pos - 1]);
//   if (!is_middle_of_word) 
//   {
//     for (;;) { // Getting to the end of the prev word
//       if (current_pos == 0) { break; }
//       if (char_is_word_char(str.data[current_pos - 1])) { break; }
//       current_pos -= 1;
//     }
//   }

//   for (;;) // Just moving to the start of the current word
//   {
//     if (current_pos == 0) { break; }
//     if (!char_is_word_char(str.data[current_pos - 1])) { break; }
//     current_pos -= 1;
//   }

//   return current_pos;
// }

// U64 __ui_move_with_control_right(Str8 str, U64 current_pos) 
// {
//   // note: CTRL+right moves to the start of a word.
//   //       If cursor is at the middle of a word, then we move to the start of the the next word that comes after the word the cursor is in the middle of.
//   //       If cursor is outside a word, then we move to the start of the first word we find by moving right.
//   //       If we reach the end of a string, then we leave the cursor there.
//   if (current_pos > str.count)
//   {
//     Assert(current_pos <= str.count); // note: Some wrong with your parameter values dude. 
//     current_pos = str.count;
//   }
//   if (current_pos == str.count) { return str.count; }

//   B32 is_middle_of_word = char_is_word_char(str.data[current_pos]);
//   if (is_middle_of_word) 
//   {
//     for (;;) { // Getting out of the current word
//       if (current_pos == str.count) { break; }
//       if (!char_is_word_char(str.data[current_pos])) { break; }
//       current_pos += 1;
//     }
//   }

//   for (;;) // Moving to the start of the next word
//   {
//     if (current_pos == str.count) { break; }
//     if (char_is_word_char(str.data[current_pos])) { break; }
//     current_pos += 1;
//   }

//   return current_pos;
// }

// UI_Text_op* ui_text_op_list_push(Arena* arena, UI_Text_op_list* list, UI_Text_op_kind kind)
// {
//   // note: The idea here is this. 
//   //       We only pass in the kind in here, since it is the only part that is shared by all the permutations
//   //       of this data structure. The fat part might be present and it might not be. 
//   //       So we leave it out to be then manually filled in by the caller. 
//   //       Kind of makes perfect sense.
//   UI_Text_op* op = ArenaPush(arena, UI_Text_op);
//   op->kind = kind;
//   DllPushBack(list, op);
//   list->count += 1;
//   return op;
// }

// UI_Text_op_list ui_text_op_list_from_rli_events(Arena* arena, RLI_Event_list* event_list)
// {
//   UI_Text_op_list result_op_list = {};
//   for (RLI_Event* ev = event_list->first; ev; ev = ev->next)
//   {
//     B32 is_keyboard_event = (ev->key_went_down || ev->key_repeat_down); 
//     if (is_keyboard_event)
//     {
//       switch (ev->key)
//       {
//         case RLI_Key__left_arrow:
//         {
//           UI_Text_op* move = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//           if (ev->modifiers & RLI_Modifier__shift) { move->keep_section_start_after_op = true; }
//           if (ev->modifiers & RLI_Modifier__control) { 
//             move->move_specifier = UI_Text_op_move_specifier___move_1_word_left; 
//           } 
//           else {  
//             move->move_specifier = UI_Text_op_move_specifier___move_1_char_left;
//             move->override_move_and_move_to_section_min_if_ending_section = true;
//           } 
//         } break;

//         case RLI_Key__right_arrow:
//         {
//           UI_Text_op* move = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//           if (ev->modifiers & RLI_Modifier__shift) { move->keep_section_start_after_op = true; }
//           if (ev->modifiers & RLI_Modifier__control) { 
//             move->move_specifier = UI_Text_op_move_specifier___move_1_word_right; 
//           } 
//           else {  
//             move->move_specifier = UI_Text_op_move_specifier___move_1_char_right;
//             move->override_move_and_move_to_section_max_if_ending_section = true;
//           } 
//         } break;

//         case RLI_Key__home:
//         {
//           UI_Text_op* op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//           op->move_specifier = UI_Text_op_move_specifier___move_to_line_start;
//           if (ev->modifiers & RLI_Modifier__shift) { op->keep_section_start_after_op = true; }
//         } break;

//         case RLI_Key__end:
//         {
//           UI_Text_op* op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//           op->move_specifier = UI_Text_op_move_specifier___move_to_line_end;
//           if (ev->modifiers & RLI_Modifier__shift) { op->keep_section_start_after_op = true; }
//         } break;

//         case RLI_Key__backspace:
//         {
//           // Section creation op 
//           {
//             UI_Text_op* move_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//             move_op->dont_move_if_section = true;
//             move_op->keep_section_start_after_op = true;
//             if (ev->modifiers & RLI_Modifier__control) { move_op->move_specifier = UI_Text_op_move_specifier___move_1_word_left; }
//             else { move_op->move_specifier = UI_Text_op_move_specifier___move_1_char_left; }
//           }

//           // Section deletion op
//           ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
//         } break;

//         case RLI_Key__delete:
//         {
//           // Section creation op 
//           {
//             UI_Text_op* move_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//             move_op->dont_move_if_section = true;
//             move_op->keep_section_start_after_op = true;
//             if (ev->modifiers & RLI_Modifier__control) { move_op->move_specifier = UI_Text_op_move_specifier___move_1_word_right; }
//             else { move_op->move_specifier = UI_Text_op_move_specifier___move_1_char_right; }
//           }

//           // Section deletion op
//           ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
//         } break;

//         default:
//         {
//           if (ev->key == RLI_Key__c && ev->modifiers & RLI_Modifier__control) // Copy
//           {
//             UI_Text_op* copy_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__copy_section);
//             copy_op->keep_section_start_after_op = true;
//           }
//           else if (ev->key == RLI_Key__v && ev->modifiers & RLI_Modifier__control) // Paste
//           {
//             ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
//             ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__paste_at_cursor);
//           }
//           else if (ev->key == RLI_Key__x && ev->modifiers & RLI_Modifier__control) // Cut
//           {
//             UI_Text_op* copy_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__copy_section);
//             copy_op->keep_section_start_after_op = true;
//             ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
//           }
//           else if (ev->key == RLI_Key__a && ev->modifiers & RLI_Modifier__control) // Select all
//           {
//             UI_Text_op* move_1 = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//             move_1->move_specifier = UI_Text_op_move_specifier___move_to_line_start;

//             UI_Text_op* move_2 = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
//             move_2->move_specifier = UI_Text_op_move_specifier___move_to_line_end;
//             move_2->keep_section_start_after_op = true;
//           }
//           else 
//           {
//             B32 is_printable = false;
//             U8 ch = u8_from_rli_key(ev->key, ev->modifiers & RLI_Modifier__shift, &is_printable); 
//             if (is_printable)
//             {
//               ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
//               UI_Text_op* insert_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__insert_char_at_cursor);
//               insert_op->char_to_insert = ch;
//             }
//           }
//         } break;
//       }
//     }
//   }
//   return result_op_list;
// }

// void ui_aply_text_ops(UI_Text_op_list text_op_list, U8* text_buffer, U64 max_text_size, U64* current_text_size, U64* cursor_pos, U64* section_start)
// {
//   for (UI_Text_op* text_op = text_op_list.first; text_op; text_op = text_op->next)
//   {
//     switch (text_op->kind)
//     { 
//       default: { } break;

//       case UI_Text_op_kind__move_cursor:
//       {
//         if (text_op->dont_move_if_section && *section_start != *cursor_pos) { goto end_of_move_op_processing; }
        
//         if (text_op->override_move_and_move_to_section_min_if_ending_section && *section_start != *cursor_pos && !text_op->keep_section_start_after_op) {
//           *cursor_pos = Min(*cursor_pos, *section_start);
//           goto end_of_move_op_processing;
//         }
//         if (text_op->override_move_and_move_to_section_max_if_ending_section && *section_start != *cursor_pos && !text_op->keep_section_start_after_op) {
//           *cursor_pos = Max(*cursor_pos, *section_start);
//           goto end_of_move_op_processing;
//         }

//         switch (text_op->move_specifier)
//         {
//           case UI_Text_op_move_specifier___move_1_char_left: { if (*cursor_pos >= 1) { *cursor_pos -= 1; } } break;
//           case UI_Text_op_move_specifier___move_1_char_right: { if (*cursor_pos < *current_text_size) { *cursor_pos += 1; } } break;

//           case UI_Text_op_move_specifier___move_1_word_left: { *cursor_pos = (U64)__ui_move_with_control_left(str8_manuall(text_buffer, *current_text_size), *cursor_pos); } break;
//           case UI_Text_op_move_specifier___move_1_word_right: { *cursor_pos = (U64)__ui_move_with_control_right(str8_manuall(text_buffer, *current_text_size), *cursor_pos); } break;

//           case UI_Text_op_move_specifier___move_to_line_start: { *cursor_pos = 0; } break;
//           case UI_Text_op_move_specifier___move_to_line_end: { *cursor_pos = *current_text_size; } break;

//           default: { } break;
//         }

//         end_of_move_op_processing: {}
//       } break;

//       case UI_Text_op_kind__delete_section:
//       {
//         if (*cursor_pos != *section_start)
//         {
//           // Deleting a part of text
//           RangeU64 range_to_delete = range_u64_make(Min(*cursor_pos, *section_start), Max(*cursor_pos, *section_start));
//           Str8 buffer_as_str = str8_manuall(text_buffer, *current_text_size);
//           Scratch scratch = get_scratch(0, 0);
//           Str8_list str_parts = {};
//           Str8 str_part_before_delete = str8_substring(buffer_as_str, 0, range_to_delete.min);
//           Str8 str_part_after_delete = str8_substring(buffer_as_str, range_to_delete.max, *current_text_size);
//           str8_list_append(scratch.arena, &str_parts, str_part_before_delete);
//           str8_list_append(scratch.arena, &str_parts, str_part_after_delete);
//           Str8 new_str = str8_from_list(scratch.arena, &str_parts);
//           Assert(new_str.count <= *current_text_size);  // It can never get bigger after deleting
//           for EachIndex(i, new_str.count) { text_buffer[i] = new_str.data[i]; }
//           *current_text_size = new_str.count;
//           end_scratch(&scratch);

//           // Moving the cursor
//           *cursor_pos = range_to_delete.min;
//         }
//       } break;

//       case UI_Text_op_kind__copy_section:
//       {
//         Scratch scratch = get_scratch(0, 0);
//         Str8 text_to_copy = str8_substring(str8_manuall(text_buffer, *current_text_size), Min(*cursor_pos, *section_start), Max(*cursor_pos, *section_start));
//         Str8 text_to_copy_nt = str8_copy_alloc(scratch.arena, text_to_copy);
//         if (text_to_copy_nt.count != 0) { SetClipboardText((char*)text_to_copy_nt.data); }
//         end_scratch(&scratch);
//       } break;

//       case UI_Text_op_kind__paste_at_cursor:
//       {
//         // Pasting 
//         Str8 buffer_as_str = str8_manuall(text_buffer, *current_text_size);
//         Scratch scratch = get_scratch(0, 0);
//         Str8_list str_parts = {};
//         Str8 str_part_before_insert = str8_substring(buffer_as_str, 0, *cursor_pos);
//         Str8 str_part_after_insert = str8_substring(buffer_as_str, *cursor_pos, *current_text_size);
//         Str8 str_to_insert = {};
//         {
//           char* clb_text = const_cast<char*>(GetClipboardText()); // todo: Remove this cpp shit here
//           str_to_insert = str8_from_cstr(scratch.arena, (U8*)clb_text);
//         }
//         str8_list_append(scratch.arena, &str_parts, str_part_before_insert);
//         str8_list_append(scratch.arena, &str_parts, str_to_insert);
//         str8_list_append(scratch.arena, &str_parts, str_part_after_insert);
//         Str8 new_str = str8_from_list(scratch.arena, &str_parts);
//         Assert(new_str.count > *current_text_size);  
//         for EachIndex(i, Min(new_str.count, max_text_size) ) { text_buffer[i] = new_str.data[i]; }
//         *current_text_size = Min(new_str.count, max_text_size);
//         end_scratch(&scratch);
      
//         // Moving the cursor
//         *cursor_pos += str_to_insert.count; // todo: This will be wrong if overflow, fix this
//       } break;

//       case UI_Text_op_kind__insert_char_at_cursor:
//       {
//         Str8 buffer_as_str = str8_manuall(text_buffer, *current_text_size);
//         Scratch scratch = get_scratch(0, 0);
//         Str8_list str_parts = {};
//         Str8 str_part_before_insert = str8_substring(buffer_as_str, 0, *cursor_pos);
//         Str8 str_part_after_insert = str8_substring(buffer_as_str, *cursor_pos, *current_text_size);
//         Str8 str_to_insert = str8_from_cstr_len(scratch.arena, &text_op->char_to_insert, 1);
//         str8_list_append(scratch.arena, &str_parts, str_part_before_insert);
//         str8_list_append(scratch.arena, &str_parts, str_to_insert);
//         str8_list_append(scratch.arena, &str_parts, str_part_after_insert);
//         Str8 new_str = str8_from_list(scratch.arena, &str_parts);
//         Assert(new_str.count > *current_text_size); // Just making sure
//         for EachIndex(i, Min(new_str.count, max_text_size) ) { text_buffer[i] = new_str.data[i]; }
//         *current_text_size = Min(new_str.count, max_text_size);
//         end_scratch(&scratch);

//         // Moving the cursor
//         if (*cursor_pos < max_text_size && *cursor_pos < u64_max) { *cursor_pos += 1; }
//       } break;
//     }

//     if (!text_op->keep_section_start_after_op) { *section_start = *cursor_pos; }

//     // Just in case
//     Assert(0 <= *cursor_pos && *cursor_pos <= *current_text_size);
//     clamp_u64_inplace(cursor_pos, 0, *current_text_size);
//   }
// }


#endif