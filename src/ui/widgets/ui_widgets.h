#ifndef __UI_WIDGETS_H
#define __UI_WIDGETS_H

#include "ui/ui_core.h"

// todo: Remove this dependancy from here
#include "render/render.h"
#include "render/render.cpp"

// - Simple widgets
void ui_label(Str8 str);
void ui_label_f(const char* fmt, ...);
void ui_spacer(UI_Size size);
UI_Actions ui_button(Str8 str_id); 
UI_Actions ui_button_f(const char* fmt, ...); 

// - Layout stack
void ui_layout_stack_begin(Axis2 axis); 
void ui_layout_stack_end();
#define UI_Stack(axis) DeferLoop(ui_layout_stack_begin(axis), ui_layout_stack_end())
#define UI_Row() UI_Stack(Axis2__x)
#define UI_Col() UI_Stack(Axis2__y)

void ui_padded_around_begin(UI_Size size)
{
  Axis2 axis = ui_get_parent()->layout_axis;
  ui_layout_stack_begin(axis2_other(axis));
  ui_spacer(size);
  ui_layout_stack_begin(axis);
  ui_spacer(size);
}

void ui_padded_around_end(UI_Size size)
{
  ui_spacer(size);
  ui_layout_stack_end();
  ui_spacer(size);
  ui_layout_stack_end();
}

// get the axis of the parent
// the padd it up

// - Padded box
#define UI_Padded(size)       DeferLoop(ui_spacer(size), ui_spacer(size))
#define UI_PaddedAround(size) DeferLoop(ui_padded_around_begin(size), ui_padded_around_end(size))

// - Color pickers
// void ui_color_picker_sv(Str8 id, UI_Size size_x, UI_Size size_y, V4F32 hsv, F32* out_opt_new_sat, F32* out_opt_new_val);
// void ui_color_picker_h(Str8 id, UI_Size size_x, UI_Size size_y, Axis2 direction, F32 hue, F32* out_opt_new_color_hsv);

// void __ui_color_picker_sv_square_draw_func(UI_Box* box);
// void __ui_color_picker_h_draw_func(UI_Box* box);

// - Image
void ui_image(R_Target texture); // note: This gets the size from the ouside: ui_set_next_size_x/y

// - Slider
struct UI_Slider_style {
  UI_Size size_x;
  UI_Size size_y;
  const char* fmt_str;
  
  V4 hover_color;
  V4 no_hover_color;
  V4 slided_part_color;
  
  // V4 text_color;
  // F32 font_size;
};
/*
B32 ui_slider(Str8 slider_id, const UI_Slider_style* slider_style, F32 current_value, F32 min, F32 max, F32* out_opt_new_value)
{
  UI_Actions slider_actions = ui_actions_from_id(slider_id);
  UI_Box_data slider_box_data = ui_get_box_data_prev_frame_from_id(slider_id);

  F32 corner_r_in_px = 4.0f;

  F32 corner_r_in_norm = 0.0f;
  if (slider_box_data.found) {
    corner_r_in_norm = corner_r_in_px / (Min(slider_box_data.on_screen_bbox.width, slider_box_data.on_screen_bbox.height) / 2);
  }

  if (slider_actions.is_hovered) { ui_set_next_b_color(slider_style->hover_color);} 
  else                           { ui_set_next_b_color(slider_style->no_hover_color); }
  ui_set_next_size_x(slider_style->size_x);
  ui_set_next_size_y(slider_style->size_y);
  ui_set_next_layout_axis(Axis2__x);
  ui_set_next_corner_r(v4f32_all(corner_r_in_norm));
  UI_Box* slider_box = ui_box_make(slider_id, UI_Box_flag__has_background|UI_Box_flag__has_rounded_corners); 

  F32 new_value = current_value;

  B32 moved_slider = false;
  if (slider_box_data.found)
  {
    F32 thumb_container_width = slider_box_data.on_screen_bbox.width;
    F32 max_thumb_offset      = thumb_container_width;
    F32 value_ratio           = (current_value - min) / (max - min);
    clamp_f32_inplace(&value_ratio, 0.0f, 1.0f);
    F32 thumb_offset = max_thumb_offset * value_ratio;
  
    UI_Parent(slider_box)
    {
      ui_set_next_size_x(ui_px(thumb_offset));
      ui_set_next_size_y(ui_px(slider_box_data.on_screen_bbox.height));
      ui_set_next_b_color(slider_style->slided_part_color);
      ui_set_next_corner_r(v4f32_all(corner_r_in_px / (Min(thumb_offset, slider_box_data.on_screen_bbox.height) / 2)));
      UI_Box* thumb_box = ui_box_make(Str8FromC("Thumb test"), UI_Box_flag__has_background|UI_Box_flag__has_rounded_corners);
    }
  
    UI_Parent(slider_box)
    {
      ui_set_next_flags(UI_Box_flag__floating);
      UI_PaddedBox(ui_p_of_p(1.0f, 0.0f), Axis2__y) 
      UI_PaddedBox(ui_p_of_p(1.0f, 0.0f), Axis2__x)
      {
        ui_label_f(slider_style->fmt_str, current_value);
      }
    }
  
    // if (slider_actions.is_hovered)
    // {
    //   current_value += slider_actions.wheel_move;
    //   clamp_f32_inplace(&current_value, min, max);
    // }
    
    if (slider_actions.is_down)
    {
      // ui_set_active_box(slider_box);
      V2F32 mouse_pos = ui_get_mouse_pos();
      thumb_offset = mouse_pos.x - slider_box_data.on_screen_bbox.x;
      moved_slider = true;
    }
    else 
    {
      // ui_reset_active_box_match(slider_box);
    }
    clamp_f32_inplace(&thumb_offset, 0.0f, max_thumb_offset);
  
    if (moved_slider)
    {
      new_value = lerp_f32(min, max, (thumb_offset / max_thumb_offset));
      if (f32_is_nan(new_value)) { BP; }
    }
  }

  if (out_opt_new_value) { *out_opt_new_value = new_value; }

  return moved_slider;
}
*/

///////////////////////////////////////////////////////////
// ---- WORK IN PROGRESS FOR A TEXT INPUT FIELD
//
F32 ui_slider(Str8 id, V2F32 dims, V4F32 b_color, V4F32 thumb_color, F32 current_value, RangeF32 value_range)
{
  clamp_f32_inplace(&current_value, value_range.min, value_range.max);
  
  ui_set_next_size_x(ui_px(dims.x));
  ui_set_next_size_y(ui_px(dims.y));
  ui_set_next_b_color(b_color);
  UI_Box* slider_main_box = ui_box_make(id, UI_Box_flag__has_background);

  F32 value_ratio = reverse_lerp_f32(value_range.min, value_range.max, current_value); 

  UI_Parent(slider_main_box)
  {
    ui_set_next_size_x(ui_px(dims.x * value_ratio));
    ui_set_next_size_y(ui_px(dims.y));
    ui_set_next_b_color(thumb_color);
    UI_Box* filled_part = ui_box_make(id, UI_Box_flag__has_background);
  }

  F32 new_value_ratio = value_ratio;
  UI_Box_data slider_main_data   = ui_box_data_from_box_prev_frame(slider_main_box);
  UI_Actions slider_main_actions = ui_actions_from_box(slider_main_box);
  if (slider_main_data.is_found)
  {
    V2F32 slider_main_box_origin = range_v2f32_x0y0(slider_main_data.on_screen_bbox);
    V2F32 slider_main_box_dims   = range_v2f32_dims(slider_main_data.on_screen_bbox);
    V2F32 mouse_pos              = ui_get_mouse_pos();
    if (slider_main_actions.is_down)
    {
      new_value_ratio = reverse_lerp_f32(slider_main_data.on_screen_bbox.min.x, slider_main_data.on_screen_bbox.max.x, mouse_pos.x); 
    }
  }

  F32 new_value = lerp_f32(value_range.min, value_range.max, new_value_ratio);
  clamp_f32_inplace(&new_value, value_range.min, value_range.max);
  return new_value;
}

///////////////////////////////////////////////////////////
// ---- WORK IN PROGRESS FOR A TEXT INPUT FIELD
//
enum UI_Text_op_kind : U32 {  
  UI_Text_op_kind__NONE,
  UI_Text_op_kind__move_cursor,
  UI_Text_op_kind__delete_section,
  UI_Text_op_kind__copy_section,
  UI_Text_op_kind__paste_at_cursor,
  UI_Text_op_kind__insert_char_at_cursor, 
  UI_Text_op_kind__stop_editing, 
};

enum UI_Text_op_move_specifier : U32 {
  UI_Text_op_move_specifier___NONE,
  UI_Text_op_move_specifier___move_1_char_left,                     
  UI_Text_op_move_specifier___move_1_char_right,                    
  UI_Text_op_move_specifier___move_1_word_left,                     
  UI_Text_op_move_specifier___move_1_word_right,                    
  UI_Text_op_move_specifier___move_to_line_start,                   
  UI_Text_op_move_specifier___move_to_line_end,          
  UI_Text_op_move_specifier___move_specific_position,                     
};

struct UI_Text_op {
  UI_Text_op_kind kind;
  
  UI_Text_op* next;
  UI_Text_op* prev;

  OS_Event* opt_os_event;

  // Fat struct data
  U8 char_to_insert;               
  B8 keep_section_start_after_op; 
  B8 dont_move_if_section;
  B8 override_move_and_move_to_section_min_if_ending_section;
  B8 override_move_and_move_to_section_max_if_ending_section;
  U64 cursor_specific_pos;
  UI_Text_op_move_specifier move_specifier;
};

struct UI_Text_op_list {
  UI_Text_op* first;
  UI_Text_op* last;
  U64 count;
};

// todo: This is here right now, but this shoud be removed and i shoud use win32 WM_CHAR events for text 
U8 u8_from_key(Key key, B32 is_shift_down, B32* out_is_printable)
{
  static B32 is_initialised = false;
  static struct {
    U8 ch;
    U8 ch_shift;
    B8 printable;
  } key_print_data_arr[Key__COUNT];

  if (!is_initialised) 
  {
    is_initialised = true;

    for (U32 i = 0; i < Key__COUNT; i++) {
      key_print_data_arr[i] = {'\0', '\0', false};
    }

    key_print_data_arr[Key__space]         = {' ',  ' ',  true};
    key_print_data_arr[Key__a]             = {'a',  'A',  true};
    key_print_data_arr[Key__b]             = {'b',  'B',  true};
    key_print_data_arr[Key__c]             = {'c',  'C',  true};
    key_print_data_arr[Key__d]             = {'d',  'D',  true};
    key_print_data_arr[Key__e]             = {'e',  'E',  true};
    key_print_data_arr[Key__f]             = {'f',  'F',  true};
    key_print_data_arr[Key__g]             = {'g',  'G',  true};
    key_print_data_arr[Key__h]             = {'h',  'H',  true};
    key_print_data_arr[Key__i]             = {'i',  'I',  true};
    key_print_data_arr[Key__j]             = {'j',  'J',  true};
    key_print_data_arr[Key__k]             = {'k',  'K',  true};
    key_print_data_arr[Key__l]             = {'l',  'L',  true};
    key_print_data_arr[Key__m]             = {'m',  'M',  true};
    key_print_data_arr[Key__n]             = {'n',  'N',  true};
    key_print_data_arr[Key__o]             = {'o',  'O',  true};
    key_print_data_arr[Key__p]             = {'p',  'P',  true};
    key_print_data_arr[Key__q]             = {'q',  'Q',  true};
    key_print_data_arr[Key__r]             = {'r',  'R',  true};
    key_print_data_arr[Key__s]             = {'s',  'S',  true};
    key_print_data_arr[Key__t]             = {'t',  'T',  true};
    key_print_data_arr[Key__u]             = {'u',  'U',  true};
    key_print_data_arr[Key__v]             = {'v',  'V',  true};
    key_print_data_arr[Key__w]             = {'w',  'W',  true};
    key_print_data_arr[Key__x]             = {'x',  'X',  true};
    key_print_data_arr[Key__y]             = {'y',  'Y',  true};
    key_print_data_arr[Key__z]             = {'z',  'Z',  true};
    key_print_data_arr[Key__0]             = {'0',  ')',  true};
    key_print_data_arr[Key__1]             = {'1',  '!',  true};
    key_print_data_arr[Key__2]             = {'2',  '@',  true};
    key_print_data_arr[Key__3]             = {'3',  '#',  true};
    key_print_data_arr[Key__4]             = {'4',  '$',  true};
    key_print_data_arr[Key__5]             = {'5',  '%',  true};
    key_print_data_arr[Key__6]             = {'6',  '^',  true};
    key_print_data_arr[Key__7]             = {'7',  '&',  true};
    key_print_data_arr[Key__8]             = {'8',  '*',  true};
    key_print_data_arr[Key__9]             = {'9',  '(',  true};
    key_print_data_arr[Key__backtick]      = {'`',  '~',  true};
    key_print_data_arr[Key__minus]         = {'-',  '_',  true};
    key_print_data_arr[Key__equals]        = {'=',  '+',  true};
    key_print_data_arr[Key__left_bracket]  = {'[',  '{',  true};
    key_print_data_arr[Key__right_bracket] = {']',  '}',  true};
    key_print_data_arr[Key__backslash]     = {'\\', '|',  true};
    key_print_data_arr[Key__semicolon]     = {';',  ':',  true};
    key_print_data_arr[Key__apostrophe]    = {'\'', '"',  true};
    key_print_data_arr[Key__comma]         = {',',  '<',  true};
    key_print_data_arr[Key__period]        = {'.',  '>',  true};
    key_print_data_arr[Key__slash]         = {'/',  '?',  true};
  }
  
  if (out_is_printable) { *out_is_printable = key_print_data_arr[key].printable; } ;
  U8 ch = key_print_data_arr[key].ch;
  if (is_shift_down) { ch = key_print_data_arr[key].ch_shift; }
  return ch;
}

U64 __ui_move_with_control_left(Str8 str, U64 current_pos) 
{
  // note: CTRL+left moves to the start of a word.
  //       If cursor is at the middle of a word, then we move to the start of the word the cursos is in the middle of.
  //       If we are outside of a word, then we move to the start of the first word we find by moving left.
  //       If we reach the start of a string, then we leave the cursor there.
  if (current_pos > str.count) { InvalidCodePath(); current_pos = str.count; }
  if (current_pos == 0) { return 0; }

  B32 is_middle_of_word = char_is_word_char(str.data[current_pos - 1]);
  if (!is_middle_of_word) 
  {
    for (;;) { // Getting to the end of the prev word
      if (current_pos == 0) { break; }
      if (char_is_word_char(str.data[current_pos - 1])) { break; }
      current_pos -= 1;
    }
  }

  for (;;) // Just moving to the start of the current word
  {
    if (current_pos == 0) { break; }
    if (!char_is_word_char(str.data[current_pos - 1])) { break; }
    current_pos -= 1;
  }

  return current_pos;
}

U64 __ui_move_with_control_right(Str8 str, U64 current_pos) 
{
  // note: CTRL+right moves to the start of a word.
  //       If cursor is at the middle of a word, then we move to the start of the the next word that comes after the word the cursor is in the middle of.
  //       If cursor is outside a word, then we move to the start of the first word we find by moving right.
  //       If we reach the end of a string, then we leave the cursor there.
  if (current_pos > str.count) { InvalidCodePath(); current_pos = str.count; }
  if (current_pos == str.count) { return str.count; }

  B32 is_middle_of_word = char_is_word_char(str.data[current_pos]);
  if (is_middle_of_word) 
  {
    for (;;) { // Getting out of the current word
      if (current_pos == str.count) { break; }
      if (!char_is_word_char(str.data[current_pos])) { break; }
      current_pos += 1;
    }
  }

  for (;;) // Moving to the start of the next word
  {
    if (current_pos == str.count) { break; }
    if (char_is_word_char(str.data[current_pos])) { break; }
    current_pos += 1;
  }

  return current_pos;
}

UI_Text_op* ui_text_op_list_push(Arena* arena, UI_Text_op_list* list, UI_Text_op_kind kind)
{
  // note: The idea here is this. 
  //       We only pass in the kind in here, since it is the only part that is shared by all the permutations
  //       of this data structure. The fat part might be present and it might not be. 
  //       So we leave it out to be then manually filled in by the caller. 
  //       Kind of makes perfect sense.
  UI_Text_op* op = ArenaPush(arena, UI_Text_op);
  op->kind = kind;
  DllPushBack(list, op);
  list->count += 1;
  return op;
}

// note: This right now creates a list from all the events that we have, which per frame is not a lot, just a couple
//       with no regard for keys that might cancel the editing of the box. Right now such key is Key__espcape.
//       Test op is created for it, but after it new ops are still made. This is not ideal, thought is fine. 
//       I am more concerned with the fact that we cant customise it right now.
UI_Text_op_list ui_text_op_list_from_os_event_list(Arena* arena, OS_Event_list* event_list)
{
  UI_Text_op_list result_op_list = {};
  
  for (OS_Event* ev = event_list->first; ev; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__key && (ev->key_event.went_down || ev->key_event.repeat_down))
    {
      switch (ev->key_event.key)
      {
        case Key__arrow_left:
        {
          UI_Text_op* move = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
          move->opt_os_event = ev;
          if (ev->key_event.modifiers & OS_Event_modifier__shift) { move->keep_section_start_after_op = true; }
          if (ev->key_event.modifiers & OS_Event_modifier__control) { 
            move->move_specifier = UI_Text_op_move_specifier___move_1_word_left; 
          } 
          else {  
            move->move_specifier = UI_Text_op_move_specifier___move_1_char_left;
            move->override_move_and_move_to_section_min_if_ending_section = true;
          } 
        } break;

        case Key__arrow_right:
        {
          UI_Text_op* move = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
          move->opt_os_event = ev; 
          if (ev->key_event.modifiers & OS_Event_modifier__shift) { move->keep_section_start_after_op = true; }
          if (ev->key_event.modifiers & OS_Event_modifier__control) { 
            move->move_specifier = UI_Text_op_move_specifier___move_1_word_right; 
          } 
          else {  
            move->move_specifier = UI_Text_op_move_specifier___move_1_char_right;
            move->override_move_and_move_to_section_max_if_ending_section = true;
          } 
        } break;

        case Key__home:
        {
          UI_Text_op* op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
          op->opt_os_event = ev;
          op->move_specifier = UI_Text_op_move_specifier___move_to_line_start;
          if (ev->key_event.modifiers & OS_Event_modifier__shift) { op->keep_section_start_after_op = true; }
        } break;

        case Key__end:
        {
          UI_Text_op* op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
          op->opt_os_event = ev;
          op->move_specifier = UI_Text_op_move_specifier___move_to_line_end;
          if (ev->key_event.modifiers & OS_Event_modifier__shift) { op->keep_section_start_after_op = true; }
        } break;

        case Key__backspace:
        {
          // Section creation op 
          {
            UI_Text_op* move_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
            move_op->dont_move_if_section = true;
            move_op->keep_section_start_after_op = true;
            move_op->move_specifier = (ev->key_event.modifiers & OS_Event_modifier__control ? 
                                       UI_Text_op_move_specifier___move_1_word_left : 
                                       UI_Text_op_move_specifier___move_1_char_left); 
          }

          // Section deletion op
          UI_Text_op* final_delete_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
          final_delete_op->opt_os_event = ev;
        } break;

        case Key__delete:
        {
          // Section creation op 
          {
            UI_Text_op* move_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
            move_op->dont_move_if_section = true;
            move_op->keep_section_start_after_op = true;
            move_op->move_specifier = (ev->key_event.modifiers & OS_Event_modifier__control ? 
                                       UI_Text_op_move_specifier___move_1_word_right : 
                                       UI_Text_op_move_specifier___move_1_char_right);
          }

          // Section deletion op
          UI_Text_op* final_delete_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
          final_delete_op->opt_os_event = ev;
        } break;

        // note: This is test code here, added this on 9th of June 2026
        case Key__escape:
        {
          UI_Text_op* stop_edit_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__stop_editing);
          stop_edit_op->opt_os_event = ev;
        } break;

        default:
        {
          if (ev->key_event.key == Key__c && ev->key_event.modifiers & OS_Event_modifier__control) // Copy
          {
            UI_Text_op* copy_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__copy_section);
            copy_op->opt_os_event = ev;
            copy_op->keep_section_start_after_op = true;
          }
          else if (ev->key_event.key == Key__v && ev->key_event.modifiers & OS_Event_modifier__control) // Paste
          {
            UI_Text_op* ______________ = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
            UI_Text_op* final_paste_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__paste_at_cursor);
            final_paste_op->opt_os_event = ev;
          }
          else if (ev->key_event.key == Key__x && ev->key_event.modifiers & OS_Event_modifier__control) // Cut
          {
            UI_Text_op* copy_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__copy_section);
            copy_op->keep_section_start_after_op = true;
            UI_Text_op* final_delete_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
            final_delete_op->opt_os_event = ev;
          }
          else if (ev->key_event.key == Key__a && ev->key_event.modifiers & OS_Event_modifier__control) // Select all
          {
            UI_Text_op* move_1 = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
            move_1->move_specifier = UI_Text_op_move_specifier___move_to_line_start;

            UI_Text_op* move_2 = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__move_cursor);
            move_2->opt_os_event = ev;
            move_2->move_specifier = UI_Text_op_move_specifier___move_to_line_end;
            move_2->keep_section_start_after_op = true;
          }
          else 
          {
            B32 is_printable = false;
            // todo: This shoud be a separate event in the win32 WM_CHAR and not this manual from key down shit
            U8 ch = u8_from_key(ev->key_event.key, ev->key_event.modifiers & OS_Event_modifier__shift, &is_printable); 
            if (is_printable && (ev->key_event.went_down || ev->key_event.repeat_down))
            {
              ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__delete_section);
              UI_Text_op* insert_op = ui_text_op_list_push(arena, &result_op_list, UI_Text_op_kind__insert_char_at_cursor);
              insert_op->opt_os_event = ev;
              insert_op->char_to_insert = ch;
            }
          }
        } break;
      }
    }
  }
  return result_op_list;
}
 
void ui_aply_text_ops(UI_Text_op_list text_op_list, U8* text_buffer, U64 max_text_size, U64* current_text_size, U64* cursor_pos, U64* section_start, B32* opt_out_did_text_change, B32* opt_out_escaped)
{
  if (opt_out_did_text_change) { *opt_out_did_text_change = false; }
  if (opt_out_escaped) { *opt_out_escaped = false; }

  B32 stop_editing = false;
  for (UI_Text_op* text_op = text_op_list.first; text_op != 0; text_op = text_op->next)
  {
    switch (text_op->kind)
    { 
      default: {} break;

      case UI_Text_op_kind__move_cursor:
      {
        if (text_op->dont_move_if_section && *section_start != *cursor_pos) { goto end_of_move_op_processing; }
        
        if (text_op->override_move_and_move_to_section_min_if_ending_section && *section_start != *cursor_pos && !text_op->keep_section_start_after_op) {
          *cursor_pos = Min(*cursor_pos, *section_start);
          goto end_of_move_op_processing;
        }
        if (text_op->override_move_and_move_to_section_max_if_ending_section && *section_start != *cursor_pos && !text_op->keep_section_start_after_op) {
          *cursor_pos = Max(*cursor_pos, *section_start);
          goto end_of_move_op_processing;
        }

        switch (text_op->move_specifier)
        {
          case UI_Text_op_move_specifier___move_1_char_left: { if (*cursor_pos >= 1) { *cursor_pos -= 1; } } break;
          case UI_Text_op_move_specifier___move_1_char_right: { if (*cursor_pos < *current_text_size) { *cursor_pos += 1; } } break;

          case UI_Text_op_move_specifier___move_1_word_left: { *cursor_pos = (U64)__ui_move_with_control_left(str8_manual(text_buffer, *current_text_size), *cursor_pos); } break;
          case UI_Text_op_move_specifier___move_1_word_right: { *cursor_pos = (U64)__ui_move_with_control_right(str8_manual(text_buffer, *current_text_size), *cursor_pos); } break;

          case UI_Text_op_move_specifier___move_to_line_start: { *cursor_pos = 0; } break;
          case UI_Text_op_move_specifier___move_to_line_end: { *cursor_pos = *current_text_size; } break;

          case UI_Text_op_move_specifier___move_specific_position: { *cursor_pos = text_op->cursor_specific_pos;  }

          default: { InvalidCodePath(); } break;
        }

        end_of_move_op_processing: {}
      } break;

      case UI_Text_op_kind__delete_section:
      {
        if (*cursor_pos != *section_start)
        {
          Scratch scratch          = get_scratch(0, 0);
          // Deleting a part of text
          RangeU64 range_to_delete = range_u64_make(Min(*cursor_pos, *section_start), Max(*cursor_pos, *section_start));
          Str8 buffer_as_str       = str8_manual(text_buffer, *current_text_size);
          
          Str8_list str_parts         = {};
          Str8 str_part_before_delete = str8_substring(buffer_as_str, 0, range_to_delete.min);
          Str8 str_part_after_delete  = str8_substring(buffer_as_str, range_to_delete.max, *current_text_size);
          str8_list_append(scratch.arena, &str_parts, str_part_before_delete);
          str8_list_append(scratch.arena, &str_parts, str_part_after_delete);
          
          Str8 new_str = str8_from_list(scratch.arena, &str_parts);
          for EachIndex(i, new_str.count) { text_buffer[i] = new_str.data[i]; }
          *current_text_size = new_str.count;
          
          // Moving the cursor
          *cursor_pos = range_to_delete.min;
          
          if (opt_out_did_text_change) { *opt_out_did_text_change = true; }

          end_scratch(&scratch);
        }
      } break;

      case UI_Text_op_kind__copy_section:
      {
        BP;
        // todo: No raylib any more, have to use clipboard from win32 now buddy
        // Scratch scratch = get_scratch(0, 0);
        // Str8 text_to_copy = str8_substring(str8_manuall(text_buffer, *current_text_size), Min(*cursor_pos, *section_start), Max(*cursor_pos, *section_start));
        // Str8 text_to_copy_nt = str8_copy_alloc(scratch.arena, text_to_copy);
        // if (text_to_copy_nt.count != 0) { SetClipboardText((char*)text_to_copy_nt.data); }
        // end_scratch(&scratch);
      } break;

      case UI_Text_op_kind__paste_at_cursor:
      {
        BP;
        // todo: No raylib any more, have to use clipboard from win32 now buddy

        // Pasting 
        // Str8 buffer_as_str = str8_manuall(text_buffer, *current_text_size);
        // Scratch scratch = get_scratch(0, 0);
        // Str8_list str_parts = {};
        // Str8 str_part_before_insert = str8_substring(buffer_as_str, 0, *cursor_pos);
        // Str8 str_part_after_insert = str8_substring(buffer_as_str, *cursor_pos, *current_text_size);
        // Str8 str_to_insert = {};
        // {
        //   char* clb_text = const_cast<char*>(GetClipboardText()); // todo: Remove this cpp shit here
        //   str_to_insert = str8_from_cstr(scratch.arena, (U8*)clb_text);
        // }
        // str8_list_append(scratch.arena, &str_parts, str_part_before_insert);
        // str8_list_append(scratch.arena, &str_parts, str_to_insert);
        // str8_list_append(scratch.arena, &str_parts, str_part_after_insert);
        // Str8 new_str = str8_from_list(scratch.arena, &str_parts);
        // Assert(new_str.count > *current_text_size);  
        // for EachIndex(i, Min(new_str.count, max_text_size) ) { text_buffer[i] = new_str.data[i]; }
        // *current_text_size = Min(new_str.count, max_text_size);
        // end_scratch(&scratch);
      
        // // Moving the cursor
        // *cursor_pos += str_to_insert.count; // todo: This will be wrong if overflow, fix this
      
        if (opt_out_did_text_change) { *opt_out_did_text_change = true; }
        
      } break;

      case UI_Text_op_kind__insert_char_at_cursor:
      {
        Scratch scratch     = get_scratch(0, 0);
        Str8 buffer_as_str  = str8_manual(text_buffer, *current_text_size);
        
        Str8_list str_parts = {};
        Str8 str_part_before_insert = str8_substring(buffer_as_str, 0, *cursor_pos);
        Str8 str_part_after_insert = str8_substring(buffer_as_str, *cursor_pos, *current_text_size);
        Str8 str_to_insert = str8_from_cstr_len(scratch.arena, &text_op->char_to_insert, 1);
        str8_list_append(scratch.arena, &str_parts, str_part_before_insert);
        str8_list_append(scratch.arena, &str_parts, str_to_insert);
        str8_list_append(scratch.arena, &str_parts, str_part_after_insert);
        
        Str8 new_str = str8_from_list(scratch.arena, &str_parts);
        for EachIndex(i, Min(new_str.count, max_text_size) ) { text_buffer[i] = new_str.data[i]; }
        *current_text_size = Min(new_str.count, max_text_size);
        
        // Moving the cursor
        if (*cursor_pos < max_text_size && *cursor_pos < u64_max) { *cursor_pos += 1; }
        
        end_scratch(&scratch);

        if (opt_out_did_text_change) { *opt_out_did_text_change = true; }
      } break;

      case UI_Text_op_kind__stop_editing:
      {
        if (opt_out_escaped) { *opt_out_escaped = true; }
        stop_editing = true;
      } break;
    }
    
    // Just in case
    Assert(0 <= *cursor_pos && *cursor_pos <= *current_text_size);
    clamp_u64_inplace(cursor_pos, 0, *current_text_size);
    
    if (!text_op->keep_section_start_after_op) { *section_start = *cursor_pos; }

    // TODO: This might fuck up the list there
    if (text_op->opt_os_event) { os_consume_frame_event(text_op->opt_os_event); }

    if (stop_editing) { break; }
  }
}
 
// This shows the text in the buffer and gives the user the means to update the data when they choose to 
UI_Text_op_list ui_text_edit_box(Arena* arena, B32 create_updates, UI_Size size_x, U8* text_buffer, U64 text_buffer_size, U64 buffer_max_count, U64 cursor_pos, U64 section_pos, Str8 edit_box_id)
{
  FP_Font font                 = ui_get_font();
  F32 cursor_size              = 2.0f;
  Str8 text_buffer_str         = str8_manual(text_buffer, text_buffer_size);
  F32 font_height              = fp_get_font_height(font);
  UI_Box_data edit_box_data    = ui_box_data_from_box_id_prev_frame(edit_box_id);
  UI_Actions edit_box_actions  = ui_actions_from_id(edit_box_id);

  Str8 str_before_cursor           = str8_substring(text_buffer_str, 0, cursor_pos);
  F32 str_before_cursor_size_in_px = fp_measure_text(str_before_cursor, font).x;

  // TODO:
  // [ ] - Slider on the side if we need to scroll
  // [ ] - Placehlder for the string inside the text edit box
  // [ ] - Shortcuts next to the command, 
  //       if the shortcut is too long, use ... for the end of it, 
  //       do the same for the command name
  // [ ] - Cursor change for the text edit
  // [ ] - Cursor change for clip box
  // [ ] - Cursor change for slider
  // [ ] - Slider should work from the middle of it

  // Figuring out clip offset for this frames edit box
  F32 new_clip_offset = 0.0f;
  if (edit_box_data.is_found)
  {
    F32 prev_edit_box_width = range_v2f32_dims(edit_box_data.on_screen_bbox).x;

    new_clip_offset = edit_box_data.clip_offset.x;
    
    // Checking if cursor is to the right of the box now
    if (str_before_cursor_size_in_px > -1.0f * edit_box_data.clip_offset.x + prev_edit_box_width) 
    {
      // Finding a cursor string substring that fits into the edit box from the end of the cursor string
      RangeU64 str_range_that_fits = range_u64_make(str_before_cursor.count, str_before_cursor.count);
      {
        F32 total_size = 0.0f;
        for (U64 i = str_before_cursor.count; i > 0 ; i -= 1)
        {
          Str8 char_sub_str     = str8_substring(str_before_cursor, i - 1, i);
          F32 char_sub_str_size = fp_measure_text(char_sub_str, font).x;
          total_size += char_sub_str_size;
          if (total_size > prev_edit_box_width) { break; }
          str_range_that_fits.min -= 1;
        }
      }

      if (range_u64_count(str_range_that_fits) > 0)
      {
        Str8 cursor_str_that_fits           = str8_substring_range(str_before_cursor, str_range_that_fits);
        F32 cursor_str_that_fits_size_in_px = fp_measure_text(cursor_str_that_fits, font).x;
        Assert(cursor_str_that_fits_size_in_px <= prev_edit_box_width);
        F32 extra_space               = prev_edit_box_width - cursor_str_that_fits_size_in_px;
        F32 offset_till_str_that_fits = str_before_cursor_size_in_px - cursor_str_that_fits_size_in_px;
        new_clip_offset = -1.0f * (offset_till_str_that_fits - extra_space + cursor_size);
      }
    }
    else if (str_before_cursor_size_in_px < -1.0f * edit_box_data.clip_offset.x)
    {
      new_clip_offset = -1.0f * str_before_cursor_size_in_px;
    }
  }
  Assert(new_clip_offset <= 0.0f);

  // Building ui for the edit box for the data that the user passed in
  ui_set_next_size_x(size_x);
  ui_set_next_size_y(ui_px(font_height));
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* edit_box = ui_box_make(edit_box_id, UI_Box_flag__clip);
  ui_box_set_clip_offset_x(edit_box, new_clip_offset);
  UI_Parent(edit_box)
  {
    ui_label(text_buffer_str);

    ui_set_next_size_x(ui_fit());
    ui_set_next_size_y(ui_px(font_height));
    ui_set_next_layout_axis(Axis2__x);
    UI_Box* cursor_section_box = ui_box_make(edit_box_id, UI_Box_flag__floating);
    UI_Parent(cursor_section_box)
    {
      U64 section_start = Min(cursor_pos, section_pos);
      U64 section_end   = Max(cursor_pos, section_pos);

      Str8 str_before_section_start = str8_substring(text_buffer_str, 0, section_start);
      Str8 str_inside_section       = str8_substring(text_buffer_str, section_start, section_end);
      Str8 str_after_section_end    = str8_substring(text_buffer_str, section_end, text_buffer_str.count);

      F32 space_before_section_start = fp_measure_text(str_before_section_start, font).x; 
      F32 space_inside_section       = fp_measure_text(str_inside_section, font).x; 
      F32 space_after_section_end    = fp_measure_text(str_after_section_end, font).x; 

      ui_spacer(ui_px(space_before_section_start));
      
      if (create_updates)
      {
        ui_set_next_size_x(ui_px(section_start == section_end ? cursor_size : space_inside_section + cursor_size));
        ui_set_next_size_y(ui_px(font_height));
        ui_set_next_b_color(blue());
        UI_Box* cursor_box = ui_box_make(edit_box_id, UI_Box_flag__has_background);
        UI_Parent(cursor_box) 
        {
          if (section_start != section_end) {
            ui_label(str_inside_section);
          }
        }
      }
    }
  }

  // Producing data for the user that represents hot to then update the text, cursor, section
  // ---
  // "create_updates" is here to allow the user to enforce the logic of when to generate updates.
  // As a result, this widget doesnt do active or anything like that, the wrapper for it
  // on the caller side will do that, this is just a flexible building block
  UI_Text_op_list result_text_op_list = {};
  if (create_updates)
  {
    if (edit_box_actions.is_down)
    {
      if (edit_box_data.is_found)
      {
        // Figuring out new cursor and new section position for when the user tried to select text 
        B32 begin_section_at_new_cursor = false;
        U64 new_cursor_pos              = cursor_pos;
        {
          F32 new_cursor_pos_in_px_in_text = ui_get_mouse_pos().x - edit_box_data.on_screen_bbox.min.x - edit_box_data.clip_offset.x;
          F32 accumulated_offset = 0.0f;
          for EachIndex(i, text_buffer_str.count)
          {
            F32 char_width       = fp_measure_text(str8_substring(text_buffer_str, i, i + 1), font).x;
            RangeF32 char_range  = range_f32_make(accumulated_offset, accumulated_offset + char_width);
            accumulated_offset += char_width;
            if (range_f32_within(char_range, new_cursor_pos_in_px_in_text))
            {
              F32 mouse_diff_inside_char = char_range.min - new_cursor_pos_in_px_in_text;
              F32 ratio = abs_f32(mouse_diff_inside_char) / range_f32_length(char_range);
              if (0.0f <= ratio && ratio <= 0.5f) { // go to the left
                new_cursor_pos = i;
              } else if (0.5f < ratio && ratio <= 1.0f) { // go to the right
                new_cursor_pos = i + 1;
              }
              if (!edit_box_actions.was_down) { begin_section_at_new_cursor = true; }
            }
          }
        }
  
        // Producing text ops for the new cursor and section positions
        if (new_cursor_pos != cursor_pos)
        {
          UI_Text_op* op = ui_text_op_list_push(arena, &result_text_op_list, UI_Text_op_kind__move_cursor);
          op->move_specifier      = UI_Text_op_move_specifier___move_specific_position;
          op->cursor_specific_pos = new_cursor_pos;
          if (!begin_section_at_new_cursor) { op->keep_section_start_after_op = true; } // Only move the section when we begin to select
        }
      }
    }
    else {
      result_text_op_list = ui_text_op_list_from_os_event_list(arena, os_get_frame_event_list());
    }
  }
  return result_text_op_list;
}

#endif