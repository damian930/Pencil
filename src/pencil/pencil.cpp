#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "__third_party/cjson/cJSON.h"
#include "__third_party/cjson/cJSON.c"

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

#include "pencil.h"

///////////////////////////////////////////////////////////
// - Main passes
//
void pencil_init(Pencil_state* P)
{
  P->frame_arena = arena_alloc(Megabytes(64), false, 0);
  
  P->current_mode = Pencil_mode__draw;
  
  P->pen_size       = 10;
  P->pen_color_hsva = hsva_from_rgba(yellow());
  P->eraser_size    = 20;

  P->draw_texures_width  = (U32)os_get_client_area_dims__unsynched().x; // todo: Handle the case when the area is negative
  P->draw_texures_height = (U32)os_get_client_area_dims__unsynched().y; // todo: Handle the case when the area is negative

  P->current_initial_draw_texture = r_make_texture(P->draw_texures_width, P->draw_texures_height);

  P->fading_texture_fade_time = 2.0f;

  // Putting everything into the free list since we already have a static buffer of draw records
  for EachIndex(i, DRAW_RECORDS_MAX_COUNT)
  {
    Draw_record* record = P->pool_of_draw_records + i;
    DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
  }

  // todo: Open a json file with shortcutes, read shortcuts, use them if present, use defaults if not, dont use at all if error
  // Shortcuts from the json settings file
  {
    Scratch scratch = get_scratch(0, 0); 
    
    Data_buffer buffer = {};
    OS_FileOpenClose(settings_file, Str8FromC("../data/settings.json"), OS_File_access__visible_read)
    {
      buffer = data_buffer_make(scratch.arena, os_file_get_props(settings_file).size);
      *ArenaPush(scratch.arena, U8) = '\0';
      os_file_read(settings_file, &buffer);
      ArenaPopType(scratch.arena, U8);
    }

    cJSON* settings_json = cJSON_Parse((char*)buffer.data);
    if (settings_json)
    {
      cJSON* shortcuts_arr = cJSON_GetObjectItemCaseSensitive(settings_json, "shortcuts");      
      if (cJSON_IsArray(shortcuts_arr))
      {
        for (int array_index = 0; array_index < cJSON_GetArraySize(shortcuts_arr); array_index += 1)
        {
          cJSON* shortcut = cJSON_GetArrayItem(shortcuts_arr, array_index);
          if (cJSON_IsObject(shortcut))
          {
            cJSON* combination = cJSON_GetObjectItemCaseSensitive(shortcut, "combination");      
            cJSON* command = cJSON_GetObjectItemCaseSensitive(shortcut, "command");      

            if (   cJSON_IsString(combination)
                && cJSON_IsString(command)
            ) {
              Str8 combination_str = str8_from_cstr_copy((U8*)cJSON_GetStringValue(combination));
              Str8 command_name_str = str8_from_cstr_copy((U8*)cJSON_GetStringValue(command));

              Str8_list str_list = str8_split(scratch.arena, combination_str, Str8FromC("+"), 0);
              if (str_list.node_count == 2)
              {
                Str8 modifier_key_str      = str_list.first->str;
                Str8 other_key_str         = str_list.last->str;
                Key modifier_key           = key_from_str8(modifier_key_str);
                Key other_key              = key_from_str8(other_key_str);
                OS_Event_modifier modifier = os_modifier_from_key(modifier_key);
                if (modifier != OS_Event_modifier__NONE && other_key != Key__NONE)
                {
                  add_shortcut(P, modifier, other_key, command_name_str);
                } else { InvalidCodePath(); }
              } else { InvalidCodePath(); }
            } else { InvalidCodePath(); }
          } else { InvalidCodePath(); }
        } 
      } else { InvalidCodePath(); }
    } else {
      // todo:
      // const char *error_ptr = cJSON_GetErrorPtr();
      // if (error_ptr != NULL) {
      //     printf("Error: %s\n", error_ptr);
      // }
    }
    cJSON_Delete(settings_json);

    P->ui_state.open_command_list = true;

    end_scratch(&scratch);
  }
}

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse)
{
  // Mouse updates
  struct Pencil_mouse_button_state {
    B32 was_up = true;
    B32 was_down;

    B32 is_up = true;
    B32 is_down;
    
    B32 went_down;
    B32 went_up;
  };
  static Pencil_mouse_button_state mouse_states[Mouse_button__COUNT] = {};
  // Ressing the mouse frame data
  for EachEnum1ToCount(Mouse_button, button)
  {
    mouse_states[button].was_up    = mouse_states[button].is_up; 
    mouse_states[button].was_down  = mouse_states[button].is_down; 
    mouse_states[button].went_down = false;
    mouse_states[button].went_up   = false;
  }
  // Updating mouse frame data based on the new os frame events
  for (OS_Event* event = os_get_frame_event_list()->first; event; event = event->next)
  {
    if (event->kind == OS_Event_kind__mouse)
    {
      Pencil_mouse_button_state* button = mouse_states + event->mouse_event.button;
      if (event->mouse_event.went_down) { button->went_down = true; button->is_down = true; button->is_up = false;}
      if (event->mouse_event.went_up) { button->went_up = true; button->is_up = true; button->is_down = false; }
      os_consume_frame_event(event);
    }
  }
  //
  //
  // Key updates
  struct Pencil_key_states {
    B32 was_up = true;
    B32 was_down;

    B32 is_up = true;
    B32 is_down;
    
    B32 went_down;
    B32 went_up;

    B32 repeat_down;
  };
  static Pencil_key_states key_states[Key__COUNT] = {};
  // Resetting the key frame data
  for EachEnum1ToCount(Key, key)
  {
    key_states[key].was_up      = key_states[key].is_up; 
    key_states[key].was_down    = key_states[key].is_down; 
    key_states[key].went_down   = false;
    key_states[key].went_up     = false;
    key_states[key].repeat_down = false;

  }
  // Updating key frame data based on the new os frame events
  for (OS_Event* event = os_get_frame_event_list()->first; event; event = event->next)
  {
    if (event->kind == OS_Event_kind__key)
    {
      Pencil_key_states* key = key_states + event->key_event.key;
      if (event->key_event.went_down) { key->went_down = true; key->is_down = true; key->is_up = false;}
      if (event->key_event.went_up) { key->went_up = true; key->is_up = true; key->is_down = false; }
      if (event->key_event.repeat_down) { key->repeat_down = true; }
      os_consume_frame_event(event);
    }
  }
  //
  //
  // Wheel stuff
  F32 wheel_scroll = 0.0f;
  for (OS_Event* event = os_get_frame_event_list()->first; event; event = event->next)
  {
    if (event->kind == OS_Event_kind__wheel)
    {
      wheel_scroll = event->wheel_event.scroll_data;
      os_consume_frame_event(event);
      break;
    }
  }

  // Running shortcuts
  for (U64 i = 0; i < P->chord_count; i += 1)
  {
    Shortcut_chord chord = P->chords[i];
    if (key_states[key_from_os_event_mod(chord.mod)].is_down && (key_states[chord.key].went_down))
    {
      run_command_from_name(P, str8_manual(chord.command_name_buffer, chord.command_name_buffer_count));
    }
  }

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
    else 
    if (P->signal_toggle_line_fade)
    {
      P->signal_toggle_line_fade = false;
      P->is_make_new_texture_fading = ToggleBool(P->is_make_new_texture_fading);
    }
  }

  if (is_ui_capturing_mouse) { return; }

  if (P->current_mode == Pencil_mode__draw)
  {
    B32 dont_start_drawing_this_frame = false;
  
    // This is fine to do and keep doing the rest of the frmae update since this is not dependand on anything and can just be plugged in
    if (wheel_scroll != 0.0f)
    {
      S64 scroll = (S64)wheel_scroll;
      S64 current_pen_size = (S64)P->pen_size;
      current_pen_size += scroll;
      clamp_s64_inplace(&current_pen_size, (S64)MIN_PEN_SIZE, (S64)MAX_PEN_SIZE);
      P->pen_size = (U32)current_pen_size;
    }
    
    if (!P->is_mid_drawing && key_states[Key__control].is_down && key_states[Key__shift].is_down && (key_states[Key__z].went_down || key_states[Key__z].repeat_down)) 
    {
      dont_start_drawing_this_frame = true; 
      
      // Getting the first next non fading record 
      Draw_record* next_record = 0;
      if      (P->current_record == 0) { next_record = P->first_record; } 
      else if (P->current_record->next != 0) { next_record = P->current_record->next; }
      for (;;) {
        if (next_record == 0) { break; }
        if (!next_record->is_fading_texture) { break; }
        P->current_record = P->current_record->next;
      }

      if (next_record) {
        P->current_record = next_record;
      }
    }
    else // User wants to remove the last line they drew
    if (!P->is_mid_drawing && key_states[Key__control].is_down && (key_states[Key__z].went_down || key_states[Key__z].repeat_down)) 
    {
      // Setting current to be the first last non fading record
      dont_start_drawing_this_frame = true;
      if (P->current_record != 0)
      {
        P->current_record = P->current_record->prev;
        for (;;) {
          if (P->current_record == 0) { break; }
          if (!P->current_record->is_fading_texture) { break; }
          P->current_record = P->current_record->prev;
        }
      }
    }
    else // User want to clear the screen
    if (!P->is_mid_drawing && key_states[Key__delete].went_down)
    {
      dont_start_drawing_this_frame = true;
  
      // Creating a new current record
      Draw_record_registration_result record_reg = register_new_draw_record(P);
      if (record_reg.succ)
      {
        record_reg.record->is_delete_texture = true;
        P->current_record = record_reg.record;
        U64 w = P->draw_texures_width;
        U64 h = P->draw_texures_height;
      }
    }
    else // User wants to start using the eraser pen 
    if (!P->is_mid_drawing && key_states[Key__r].went_down)
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
    if (!P->is_mid_drawing && key_states[Key__b].went_down)
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
    if (key_states[Key__tab].went_down)
    {
      P->show_brush_ui_menu = ToggleBool(P->show_brush_ui_menu);
    }
  
    if (dont_start_drawing_this_frame) { goto __active_draw_update_routine_end__; }
  
    for (Draw_record* record = P->first_record; record != 0; record = record->next)
    {
      Assert(!(P->is_mid_drawing && P->current_record->is_fading_texture)); // Fading is only aplied for the once that you have finished drawing, so they dont get deleted mid draw, this assert here is for that
      
      if (record->is_fading_texture)
      {
        record->time_left_till_full_fade_sec -= os_get_time_since_last_frame();
        if (record->time_left_till_full_fade_sec <= 0.0f) 
        { 
          if (P->current_record == record) { P->current_record = P->current_record->prev; }
          delete_draw_record__invalidates_record(P, record);
        }
      }
    }

    // Starting a new draw record
    if (!P->is_mid_drawing && (mouse_states[Mouse_button__left].is_down || mouse_states[Mouse_button__right].is_down)) 
    {
      Draw_record_registration_result record_registation = register_new_draw_record(P);
      if (record_registation.succ)
      {
        P->is_mid_drawing = true;
        P->current_record = record_registation.record;
        if (mouse_states[Mouse_button__right].is_down) { 
          // todo/note: We dont really need the mode to be the eraser, its fine to just have a texture be the erasing texture and we done
          P->is_erasing_mode = true; 
          P->is_erasing_mode_for_a_single_drawing = true;
          record_registation.record->is_eraser_texture = true;
        }
      }
    }
    else // Updating active drawing 
    if (   P->is_mid_drawing 
        && (
              (!P->is_erasing_mode_for_a_single_drawing && mouse_states[Mouse_button__left].is_down)
           || (P->is_erasing_mode_for_a_single_drawing && mouse_states[Mouse_button__right].is_down)
          )
    ) {
      Assert(P->current_record != 0);

      V2F32 new_pos  = os_get_mouse_pos();
      V2F32 prev_pos = os_get_prev_mouse_pos();
      
      V4F32 color_rgba = rgba_from_hsva(P->pen_color_hsva);
      F32 pen_size = (F32)P->pen_size;
      
      if (P->is_erasing_mode) { 
        color_rgba = magenta(); 
        pen_size = (F32)P->eraser_size; 
      }
  
      D_RenderTarget(P->current_record->drawing_texture)
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
    }
    else // Here we finalise the draw record that the user have been drawing
    if (   P->is_mid_drawing 
        && (
              (!P->is_erasing_mode_for_a_single_drawing && mouse_states[Mouse_button__left].went_up)
           || (P->is_erasing_mode_for_a_single_drawing && mouse_states[Mouse_button__right].went_up)
          )
    ) {
      Assert(P->current_record != 0);
      P->is_mid_drawing = false;
    
      P->current_record->time_left_till_full_fade_sec = P->fading_texture_fade_time;
      P->current_record->is_fading_texture = P->is_make_new_texture_fading;

      if (P->is_erasing_mode_for_a_single_drawing) { 
        P->is_erasing_mode = false;
        P->is_erasing_mode_for_a_single_drawing = false; 
      }
    }
  
    __active_draw_update_routine_end__: {};
  }
  else
  if (P->current_mode == Pencil_mode__ruler)
  {
    if (!P->is_mid_ruling)
    {
      if (mouse_states[Mouse_button__left].went_down)
      {
        P->is_mid_ruling    = true;
        P->ruling_start_pos = os_get_mouse_pos();
        P->ruling_end_pos   = os_get_mouse_pos();
      }
      else if (mouse_states[Mouse_button__right].went_down) // Resetting the ruler to remove it from the screen
      {
        P->ruling_start_pos = {};
        P->ruling_end_pos   = {};
      }
    }
    else // Updating the ruling 
    if (P->is_mid_ruling)
    {
      // Condition to end the ruling
      if (mouse_states[Mouse_button__left].is_up) {
        P->is_mid_ruling = false;
      } 
      else {
        P->ruling_end_pos = os_get_mouse_pos();
      }
    }
  }
  /*
  else 
  if (P->current_mode == Pencil_mode__temp_texture)
  {
    if (!P->is_mid_drawing && mouse_states[Mouse_button__left].is_down)
    {
      P->is_mid_drawing = true;
      P->temp_drawing_texture = r_make_texture(P->draw_texures_width, P->draw_texures_height);
      P->temp_texture_initial_time_to_fade = 1.0f;
      P->temp_texture_time_left_to_fade = 1.0f;
    }
    else if (P->is_mid_drawing && mouse_states[Mouse_button__left].is_down)
    {
      D_RenderTarget(P->temp_drawing_texture)
      {
        d_draw_circle(os_get_mouse_pos(), (F32)P->pen_size, rgba_from_hsva(P->pen_color_hsva), 2.0f);
      }
    }
    else if (P->is_mid_drawing && mouse_states[Mouse_button__left].is_up)
    {
      P->is_mid_drawing = false;
    }
  }
  */
}

void pencil_render_draw_record(const Pencil_state* P, const Draw_record* record, B32 ignore_fading_records)
{
  if (ignore_fading_records && record->is_fading_texture) { return; }

  Rect texture_rect = rect_make(0.0f, 0.0f, (F32)P->draw_texures_width, (F32)P->draw_texures_height);
  if (record->is_delete_texture) {
    D_BlendKind(R_Blend_kind__no_blend) { d_fill_with_color(transparent()); }
  } 
  else if (record->is_fading_texture) {
    V4F32 fading_tint = white();
    fading_tint.a = (record->time_left_till_full_fade_sec * record->time_left_till_full_fade_sec * record->time_left_till_full_fade_sec / P->fading_texture_fade_time);
    d_draw_texture_pro(record->drawing_texture, texture_rect, texture_rect, fading_tint);
  }
  else if (record->is_eraser_texture)
  {
    D_BlendKind(R_Blend_kind__dest_out) { d_draw_texture_pro(record->drawing_texture, texture_rect, texture_rect, white()); }
  }
  else {
    d_draw_texture_pro(record->drawing_texture, texture_rect, texture_rect, white());
  }
}

void pencil_render(const Pencil_state* P)
{
  Rect texture_rect = rect_make(0.0f, 0.0f, (F32)P->draw_texures_width, (F32)P->draw_texures_height);
  d_draw_texture_pro(P->current_initial_draw_texture, texture_rect, texture_rect, white());
  
  if (P->current_record != 0)
  {
    for (Draw_record* record = P->first_record; record != 0 && record != P->current_record->next; record = record->next)
    {
      pencil_render_draw_record(P, record, false);
    }
  }

  // Rendering the ruller
  {
    if (P->current_mode == Pencil_mode__ruler)
    {
      Rect ruler_rect = rect_from_range_v2f32(range_v2f32_as_bb(P->ruling_start_pos, P->ruling_end_pos));
      d_draw_rect_inset_borders(ruler_rect, red(), 2.0f, v4f32_all(0.0f), 0.0f);
  
      V2F32 ruler_rect_center = rect_get_center(ruler_rect);
      V2F32 reler_dims        = ruler_rect.dims;
      FP_Font font            = {};//ui_get_font();
      V2F32 text_pos          = v2f32_sub(ruler_rect_center, v2f32(0.0f, (fp_get_font_height(font) / 2.0f))); 
  
      d_draw_circle(ruler_rect_center, 3, green(), 2.0f);
      d_draw_text_f("W: %.0f, H: %.0f", font, text_pos, white(), reler_dims.x, reler_dims.y);
    }
  }

}

void pencil_do_command_ui(Pencil_state* P, FP_Font font)
{
  if (!P->ui_state.open_command_list) { return; }

  // Just putting this here for now, but this might be a part of 
  // some ui state needed for the pencil ui
  static struct {
    B32 is_widget_open = true;
    
    U8 text_entry_buffer[64];
    U64 text_entry_buffer_count;
    U64 text_entry_cursor;
    U64 text_entry_section;

    U64 currently_chosen_command_index;

    V4F32 main_b_color = rgba_from_hex(0x695A09FF);
  } ui_state = {};

  if (!ui_state.is_widget_open) { return; }

  Scratch scratch = get_scratch(0, 0);
  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos());
  ui_push_font(font);

  // TODO: Put this in a better place
  static B32 navigated_commands_with_arrows = false;

  // Gathered data from the upcomming widget build 
  // to then update the state coherently
  UI_Text_op_list text_entry_ops = {};
  B32 command_got_chosen         = false;
  U64 chosen_command_index       = 0;

  // Preparing some data for later use
  struct Command_data {
    U64 index;
    B32 is_filtered_in;
  };
  Command_data filtered_command_data[ArrayCount(command_names)] = {};
  {
    Str8 entry_text = str8_manual(ui_state.text_entry_buffer, ui_state.text_entry_buffer_count);
    for EachIndex(i, ArrayCount(command_names))
    {
      filtered_command_data[i].index          = i;
      if (str8_is_substring(command_names[i], entry_text, Str8_match__ignore_case)) 
      {
        filtered_command_data[i].is_filtered_in = true;
      }
    }
  }

  // Building the widget 
  ui_layout_x();
  UI_Row() UI_Padded(ui_p_of_p(1, 0))
  {
    ui_width(ui_p_of_p(0.5, 1));
    ui_height(ui_p_of_p(0.5, 1));
    ui_b_color(ui_state.main_b_color);
    ui_layout_x();
    UI_Box* box = ui_box_make({}, UI_Box_flag__has_background);
    UI_Parent(box) UI_Padded(ui_px(10)) UI_Col() UI_Padded(ui_px(10))
    {
      Str8 text_entry_str = str8_manual(ui_state.text_entry_buffer, ui_state.text_entry_buffer_count);

      ui_width(ui_p_of_p(1, 0));
      ui_height(ui_fit());
      ui_border(1, white());
      UI_Parent(ui_box_make({}, UI_Box_flag__has_borders)) UI_PaddedAround(ui_px(5))
      {
        text_entry_ops = ui_text_edit_box(
          scratch.arena,
          ui_p_of_p(1, 0),
          ui_state.text_entry_buffer,
          ui_state.text_entry_buffer_count,
          ArrayCount(ui_state.text_entry_buffer),
          ui_state.text_entry_cursor,
          ui_state.text_entry_section,
          Str8FromC("Commands text entry box id")
        );              
      }

      ui_spacer(ui_px(5));

      ui_width(ui_p_of_p(1, 0));
      ui_height(ui_p_of_p(1, 0));
      ui_border(1, white());
      UI_Box* clip_box = ui_box_make(Str8FromC("Command list clip box id"), UI_Box_flag__clip|UI_Box_flag__has_borders);
      UI_Parent(clip_box) UI_PaddedAround(ui_px(5))
      {
        for EachIndex(command_index, ArrayCount(filtered_command_data))
        {
          Command_data command_data = filtered_command_data[command_index]; 

          if (command_data.is_filtered_in)
          {
            ui_layout_x();
            UI_Box* command_entry_box = ui_box_make_f("Command row entry id __ %lld", UI_Box_flag__has_background, command_data.index);
            UI_Parent(command_entry_box)
            {
              ui_label(command_names[command_data.index]);
              ui_spacer(ui_p_of_p(1, 0));
            }
            
            UI_Actions entry_acts = ui_actions_from_box(command_entry_box);
            if (entry_acts.is_hovered || ui_state.currently_chosen_command_index == command_index) { 
              ui_set_b_color(command_entry_box, color_light_up(ui_state.main_b_color, 0.25)); 
            }
            if (entry_acts.is_clicked) { 
              command_got_chosen   = true;
              chosen_command_index = command_index; 
            }
          }
        }
      }

      // Updating the ui state 
      UI_Actions clip_box_acts  = ui_actions_from_box(clip_box);
      UI_Box_data clip_box_data = ui_box_data_from_box_prev_frame(clip_box);

      // Adjusting the clip offset 
      F32 clip_offset = ui_box_get_prev_build_clip_offset(clip_box).y;
      if (clip_box_data.is_found)
      {
        if (navigated_commands_with_arrows)
        {
          Str8 com_index_id             = str8_fmt(scratch.arena, "Command row entry id __ %lld", ui_state.currently_chosen_command_index);
          UI_Box_data chosen_entry_data = ui_box_data_from_box_id_prev_frame(com_index_id);
          if (chosen_entry_data.is_found)
          {
            RangeF32 visible_clip_box_on_screen_y_range = range_v2f32_y0y1(clip_box_data.on_screen_bbox);
            RangeF32 entry_on_screen_y_range            = range_v2f32_y0y1(chosen_entry_data.on_screen_bbox);
            if (!range_f32_contains_range(visible_clip_box_on_screen_y_range, entry_on_screen_y_range)) 
            {
              if (entry_on_screen_y_range.min < visible_clip_box_on_screen_y_range.min)
              {
                // Entry is above the clip box top — snap its top to the clip top
                clip_offset += entry_on_screen_y_range.min - visible_clip_box_on_screen_y_range.min;
              }
              else
              {
                // Entry is below the clip box bottom — snap its bottom to the clip bottom
                clip_offset += entry_on_screen_y_range.max - visible_clip_box_on_screen_y_range.max;
              }
            }
          }
        }
        else 
        if (clip_box_acts.is_hovered)
        {
          for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
          {
            if (ev->kind == OS_Event_kind__wheel)
            {
              clip_offset += ev->wheel_event.scroll_data * 10;
              os_consume_frame_event(ev);
              break;
            }
          }
        }

        F32 space_after_vp = clip_box_data.inner_content_dims.y - range_v2f32_dims(clip_box_data.on_screen_bbox).y;
        if (space_after_vp < 0.0f) { space_after_vp = 0.0f; }
        if (-clip_offset > space_after_vp) { clip_offset = -space_after_vp; }
        if (clip_offset > 0.0f) { clip_offset = 0.0f; }
      }
      ui_box_set_clip_offset_y(clip_box, clip_offset);
    }
  }

  // Resseting
  navigated_commands_with_arrows = false;

  // Updating data 
  {
    ui_aply_text_ops(
      text_entry_ops, 
      ui_state.text_entry_buffer, 
      ArrayCount(ui_state.text_entry_buffer), 
      &ui_state.text_entry_buffer_count, 
      &ui_state.text_entry_cursor, 
      &ui_state.text_entry_section, 
      0, 0 
    );

    // Cycling over the the filtered commands
    for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__key && (ev->key_event.went_down || ev->key_event.repeat_down) && ev->key_event.key == Key__arrow_up)
      {
        navigated_commands_with_arrows = true;
        for EachIndex(i, ArrayCount(filtered_command_data))
        {
          ui_state.currently_chosen_command_index -= 1;
          clamp_u64_inplace(&ui_state.currently_chosen_command_index, 0, ArrayCount(command_names) - 1);
          if (filtered_command_data[ui_state.currently_chosen_command_index].is_filtered_in) {
            break;
          }
        }
        os_consume_frame_event(ev);
      }
      else
      if (ev->kind == OS_Event_kind__key && (ev->key_event.went_down || ev->key_event.repeat_down) && ev->key_event.key == Key__arrow_down)
      {
        navigated_commands_with_arrows = true;
        for EachIndex(i, ArrayCount(filtered_command_data))
        {
          ui_state.currently_chosen_command_index += 1;
          if (ui_state.currently_chosen_command_index == ArrayCount(command_names)) { ui_state.currently_chosen_command_index = 0; }
          if (filtered_command_data[ui_state.currently_chosen_command_index].is_filtered_in) {
            break;
          }
        }
        os_consume_frame_event(ev);
      }
      else
      if (ev->kind == OS_Event_kind__key && ev->key_event.went_down && ev->key_event.key == Key__enter)
      {
        command_got_chosen = true;
        chosen_command_index = ui_state.currently_chosen_command_index;
      }
    }

    if (command_got_chosen)
    {
      Str8 command_name = command_names[chosen_command_index];
      // run_command_from_name(P, command_name);
      char* cstr = cstr_from_str8(scratch.arena, command_name);
      OutputDebugStringF("Command: %s \n", cstr);
      
      // todo: Reset the state here
    }
  }

  ui_pop_font();
  ui_end_build();
  end_scratch(&scratch);
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
      Draw_record* prev_record = record->prev;
      delete_draw_record__invalidates_record(P, record);
      record = prev_record;
    }
  }

  Draw_record* new_draw_record = __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(P);
  if (new_draw_record == 0)
  {
    // We didnt get the new draw record, so we have to delete the oldest one. 
    // Before deleting the oldest one we have to store its drawing into the intial texture state
    // Changing the initial texture 
    D_RenderTarget(P->current_initial_draw_texture)
    {
      // note: This batches it and delayes it till the r_submit call. 
      //       Leaving it like this right now, but might at some point have a need to have intermediate 
      //       draws
      pencil_render_draw_record(P, P->first_record, true);
    }

    Draw_record* oldest_record = P->first_record;
    DllPopFront_Name(P, first_record, last_record, next, prev);
    
    *oldest_record = Draw_record{};
    DllPushBack_Name(P, oldest_record, first_free_draw_record, last_free_draw_record, next, prev);

    new_draw_record = __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(P);
  }
  Assert(new_draw_record != 0); 

  // Adding the new draw record to the draw record queue
  DllPushBack_Name(P, new_draw_record, first_record, last_record, next, prev); Assert(P->last_record == new_draw_record);

  new_draw_record->drawing_texture = r_make_texture(P->draw_texures_width, P->draw_texures_height);
  // todo: HandleLater(new_draw_record->texture_before_we_affected != 0);

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

void delete_draw_record__invalidates_record(Pencil_state* P, Draw_record* record_to_delete)
{
  if (P->is_mid_drawing && P->current_record == record_to_delete) { BP; return; }

  r_release_texture(&record_to_delete->drawing_texture);

  DllPop_Name(P, record_to_delete, first_record, last_record, next, prev);
  *record_to_delete = Draw_record{};
  DllPushBack_Name(P, record_to_delete, first_free_draw_record, last_free_draw_record, next, prev);
}

void add_shortcut(Pencil_state* P, OS_Event_modifier mod, Key key, Str8 command_name)
{
  Shortcut_chord* chord = 0;

  // Might have the command or chord already set, so we will reset it 
  for (U64 i = 0; i < P->chord_count; i += 1)
  {
    Shortcut_chord* test_chord = P->chords + i;
    if (   str8_match(str8_manual(test_chord->command_name_buffer, test_chord->command_name_buffer_count), command_name, 0) 
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
      chord->mod = mod;
      chord->key = key;
      for (U64 i = 0; i < Min(command_name.count, ArrayCount(chord->command_name_buffer)); i += 1) { chord->command_name_buffer[i] = command_name.data[i]; }
      chord->command_name_buffer_count = Min((U8)command_name.count, ArrayCount(chord->command_name_buffer));
      // todo: This cuts off the command names for now
    }
  }
}

// TODO: This shoud have a better name for this, this is kind of confusing
void run_command_from_name(Pencil_state* P, Str8 command_name)
{
  if (0) {}
  else if (str8_match(command_name, command_names[Command_id__terminate_app], Str8_match__ignore_case)) { command_terminate_app(P); }
  else if (str8_match(command_name, command_names[Command_id__swap_to_ruler], Str8_match__ignore_case)) { command_swap_to_ruller(P); }
  else if (str8_match(command_name, command_names[Command_id__swap_to_draw],  Str8_match__ignore_case)) { command_swap_to_draw(P); }
  else if (str8_match(command_name, command_names[Command_id__toggle_line_fade], Str8_match__ignore_case)) { command_toggle_line_fade(P); }
  else if (str8_match(command_name, command_names[Command_id__swap_to_eraser], Str8_match__ignore_case)) { command_swap_to_eraser(P); }
  else if (str8_match(command_name, command_names[Command_id__make_background_blue], Str8_match__ignore_case)) { command_make_background_blue(P); }
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

void command_toggle_line_fade(Pencil_state* P)
{
  P->signal_toggle_line_fade = true;
}

void command_swap_to_eraser(Pencil_state* P)
{
  P->signal_swap_to_eraser = true;
}

void command_make_background_blue(Pencil_state* P)
{
  P->signal_make_b_blue = true;
}

void command_open_command_list(Pencil_state* P)
{
  P->ui_state.open_command_list = true;
}


B32 is_valid_command_name(Str8 command_name)
{
  B32 result = false;
  for EachIndex(i, ArrayCount(command_names))
  {
    if (str8_match(command_name, command_names[i], Str8_match__ignore_case)) {
      result = true;
      break;
    }
  }
  return result;
}


// todo: I would like to pass P here as const, and signals as a separate thing then to have it clear that ui doesnt modify the state at all
#if DEBUG_MODE
/*
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
    r_export_texture(P->, Str8FromC("always_fresh_texture.png"));
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
*/
#endif

#endif