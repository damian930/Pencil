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
  // if (actions.is_down) { ui_set_active_box(box); }
  // else if (!actions.is_down) { ui_reset_active_box_match(box); } // todo: This shoud also check if it is active, cause then if we just down on the button it will reset the global active state
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
struct _UI_Color_picker_sv_data {
  V4F32 colors[UV__COUNT];
};

struct _UI_Color_picker_h_data {
  Axis2 axis;
};

void ui_color_picker_sv(Str8 id, UI_Size size_x, UI_Size size_y, V4F32 hsva, F32* out_opt_new_sat, F32* out_opt_new_val)
{
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

  // note: Plan on how the state works here:
  // 1) Do the ui based on the current state of the data
  //    - Draw the picker based on the provider prev color
  // 2) Based on the inputs, update the data and just give the new data to the user, keep the old one
  //    - Based on mouse pos + color picker dims, get the new color, return to the user
  
  // Setting up the color picke box
  ui_set_next_size_x(size_x);
  ui_set_next_size_y(size_y);
  UI_Box* color_picker_box = ui_box_make(id, 0);

  V4F32 pure_hsv = v4f32(hsva.hue, 1.0f, 1.0f, 1.0f);

  _UI_Color_picker_sv_data* draw_data = ArenaPush(ui_get_build_arena(), _UI_Color_picker_sv_data);
  draw_data->colors[UV__00] = white(); 
  draw_data->colors[UV__01] = black();
  draw_data->colors[UV__10] = rgb_from_hsv(pure_hsv);
  draw_data->colors[UV__11] = black();
  ui_box_set_custom_draw(color_picker_box, __ui_color_picker_sv_square_draw_func, draw_data);
  
  UI_Box_data box_data = ui_get_box_data_prev_frame_from_box(color_picker_box);
  V2F32 mouse          = ui_get_mouse_pos();
  F32 circle_diameter  = 15.0f;

  F32 circle_x_offset = 0.0f;
  F32 circle_y_offset = 0.0f;
  if (box_data.found)
  {
    // Reverse lerp
    F32 x_t = hsva.saturation;
    F32 y_t = hsva.value ;
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

        ui_set_next_size_x(ui_px(circle_diameter));
        ui_set_next_size_y(ui_px(circle_diameter));
        ui_set_next_corner_r(ui_corner_r_all(1));
        ui_set_next_border(4, white());
        ui_set_next_softness(1.5f);
        UI_Box* circle_picker = ui_box_make(Str8{}, 0);
      }
    }
  }

  // Updating the colors 
  F32 new_sat = hsva.saturation;
  F32 new_val = hsva.value;
  UI_Actions actions = ui_actions_from_box(color_picker_box);
  if (!actions.is_down) {
    // ui_reset_active_box_match(color_picker_box); 
    os_set_cursor(actions.is_hovered ? OS_Cursor__hand : OS_Cursor__arrow);
  }
  else {
    // ui_set_active_box(color_picker_box);
    os_set_cursor(OS_Cursor__hand);
    
    if (box_data.found)
    {
      F32 picker_relative_x = (mouse.x - box_data.on_screen_rect.x) / (box_data.on_screen_rect.width);
      F32 picker_relative_y = 1.0f - ((mouse.y - box_data.on_screen_rect.y) / (box_data.on_screen_rect.height)); // Flipping the Y since color picker is bottom_left->up and the screen is top_left->down
      clamp_f32_inplace(&picker_relative_x, 0.0f, 1.0f);
      clamp_f32_inplace(&picker_relative_y, 0.0f, 1.0f);

      // Getting the color for the realative mouse pos
      new_sat = picker_relative_x;
      new_val = picker_relative_y;
    }
  }
  if (out_opt_new_sat) { *out_opt_new_sat = new_sat; }
  if (out_opt_new_val) { *out_opt_new_val = new_val; }
}

void ui_color_picker_h(Str8 id, UI_Size size_x, UI_Size size_y, Axis2 direction, F32 hue, F32* out_opt_new_color_hsv)
{
  ui_set_next_size_x(size_x);
  ui_set_next_size_y(size_y);
  UI_Box* color_picker_box = ui_box_make(id, 0);

  _UI_Color_picker_h_data* draw_data = ArenaPush(ui_get_build_arena(), _UI_Color_picker_h_data);
  draw_data->axis = direction;
  ui_box_set_custom_draw(color_picker_box, __ui_color_picker_h_draw_func, draw_data);

  UI_Box_data box_data = ui_get_box_data_prev_frame_from_box(color_picker_box);
  V2F32 box_origin     = rect_get_origin(box_data.on_screen_rect);
  V2F32 box_dims       = rect_get_dims(box_data.on_screen_rect);

  F32 thumb_width  = 10.0f;
  F32 thumb_offset = 0.0f;
  if (box_data.found)
  { 
    thumb_offset = hue * box_dims.v[direction] - (thumb_width / 2.0f);
  }

  UI_Parent(color_picker_box)
  {
    UI_Stack(direction)
    {
      ui_spacer(ui_px(thumb_offset));

      if (box_dims.x != 0.0f)
      {
        ui_set_next_size_x(ui_px(thumb_width));
        ui_set_next_size_y(ui_px(box_dims.v[axis2_other(direction)]));
      }
      ui_set_next_corner_r(ui_corner_r_all(0.25));
      ui_set_next_border(4, white());
      UI_Box* thumb = ui_box_make({}, 0);
    }
  }

  F32 new_hue = hue;
  UI_Actions actions = ui_actions_from_box(color_picker_box);
  if (!actions.is_down)
  {
    // ui_reset_active_box_match(color_picker_box);
  }
  else if (actions.is_down)
  {
    // ui_set_active_box(color_picker_box);
    if (box_data.found)
    {
      F32 mouse_pos_rel = ui_get_mouse_pos().v[direction] - box_origin.v[direction] - (thumb_width / 2.0f);
      new_hue = mouse_pos_rel / box_dims.v[direction];
      clamp_f32_inplace(&new_hue, 0.0f, 1.0f);
    }
  }

  if (out_opt_new_color_hsv) { *out_opt_new_color_hsv = new_hue; }
}

void __ui_color_picker_sv_square_draw_func(UI_Box* box)
{
  _UI_Color_picker_sv_data* data = (_UI_Color_picker_sv_data*)box->custom_draw_data;
  d_add_rect_command_ex(box->final_on_screen_rect, data->colors, {}, {}, {});
}

void __ui_color_picker_h_draw_func(UI_Box* box)
{
  _UI_Color_picker_h_data* data = (_UI_Color_picker_h_data*)box->custom_draw_data;

  // Itteration over the 6 color section of hue
  F32 hue_section_start  = 0.0f;
  F32 hue_section_length = 1.0f / 6.0f;
  for EachIndex(i, 6)
  {
    V4F32 rgb_for_section_start = rgb_from_hsv(v4f32(hue_section_start, 1.0f, 1.0f, 1.0f));
    V4F32 rgb_for_section_end   = rgb_from_hsv(v4f32(hue_section_start + hue_section_length, 1.0f, 1.0f, 1.0f));
    
    V4F32 colors[UV__COUNT] = {};
    colors[UV__00] = data->axis == Axis2__x ? rgb_for_section_start : rgb_for_section_start;
    colors[UV__10] = data->axis == Axis2__x ? rgb_for_section_end   : rgb_for_section_start;
    colors[UV__01] = data->axis == Axis2__x ? rgb_for_section_start : rgb_for_section_end;
    colors[UV__11] = data->axis == Axis2__x ? rgb_for_section_end   : rgb_for_section_end;
    
    V4F32 corner_r = {};
    if (i == 0) 
    { 
      UV corner = (data->axis == Axis2__x ? UV__01 : UV__10);
      corner_r.v[UV__00] =  box->shape_style.corner_r.r.v[UV__00]; 
      corner_r.v[corner] = box->shape_style.corner_r.r.v[corner];
    }
    else if (i == 5)
    {
      UV corner = (data->axis == Axis2__x ? UV__10 : UV__01);
      corner_r.v[UV__11] =  box->shape_style.corner_r.r.v[UV__11]; 
      corner_r.v[corner] = box->shape_style.corner_r.r.v[corner];
    }

    Rect rect = {};
    {
      V2F32 box_p    = rect_get_origin(box->final_on_screen_rect);
      V2F32 box_dims = rect_get_dims(box->final_on_screen_rect);
  
      V2F32 rect_p = {};
      rect_p.v[data->axis]              = hue_section_start * box_dims.v[data->axis] + box_p.v[data->axis]; 
      rect_p.v[axis2_other(data->axis)] = box_p.v[axis2_other(data->axis)];
      
      V2F32 rect_dims = {};
      rect_dims.v[data->axis]              = hue_section_length * box_dims.v[data->axis];
      rect_dims.v[axis2_other(data->axis)] = box_dims.v[axis2_other(data->axis)];

      rect = rect_make(rect_p.x, rect_p.y, rect_dims.x, rect_dims.y);
    }

    d_add_rect_command_ex(rect, colors, corner_r, {}, box->shape_style.softness);

    hue_section_start += hue_section_length;
  }
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