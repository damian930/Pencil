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

extern global OS_File g_pencil_run_log_file = {}; 

void pencil_log_run_data(Str8 str)
{
  Assert(os_file_is_valid(g_pencil_run_log_file));
  B32 succ = true;
  succ &= os_file_write_end(g_pencil_run_log_file, str);
  if (str.count > 0 && str.data[str.count - 1] != '\0') { 
    succ &= os_file_write_end(g_pencil_run_log_file, Str8FromC("\n"));
  }
  Assert(succ);
}

///////////////////////////////////////////////////////////
// - Main passes
//
void pencil_init__parse_settings_json(Pencil_state* P);

void pencil_init(Pencil_state* P)
{
  g_pencil_run_log_file = os_file_open(Str8FromC("run_log_file.txt"), OS_File_access__read|OS_File_access__write|OS_File_access__share_read);
  if (!os_file_is_valid(g_pencil_run_log_file))
  {
    // TODO: The app should not run here
  }

  P->frame_arena  = arena_alloc(Megabytes(64), false, 0);
  P->current_mode = Pencil_mode__draw;
  
  P->pen_size       = 10;
  P->pen_color_hsva = hsva_from_rgba(yellow());
  P->eraser_size    = 20;

  // TODO:
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

  pencil_init__parse_settings_json(P);
}

// TODO: Have this fail and do something regarding the error, at least have it in string or something
void pencil_init__parse_settings_json(Pencil_state* P)
{
  Scratch scratch = get_scratch(0, 0); 

  // Trying to parse json
  cJSON* settings_json = 0;
  {
    B32 buffer_read_succ   = {};
    Data_buffer buffer     = {};
    OS_FileOpenClose(settings_file, Str8FromC("../data/settings.json"), OS_File_access__visible_read)
    {
      buffer = data_buffer_make(scratch.arena, os_file_get_props(settings_file).size);
      *ArenaPush(scratch.arena, U8) = '\0';
      buffer_read_succ = os_file_read(settings_file, &buffer);
    }
    if (!buffer_read_succ) { goto json_parsing_end; }

    settings_json = cJSON_Parse((char*)buffer.data);
    if (!settings_json) { goto json_parsing_end; }
    
    cJSON* shortcuts_json_arr = cJSON_GetObjectItemCaseSensitive(settings_json, "shortcuts");      
    if (!shortcuts_json_arr)               { goto json_parsing_end; }
    if(!cJSON_IsArray(shortcuts_json_arr)) { goto json_parsing_end; }

    for (int array_index = 0; array_index < cJSON_GetArraySize(shortcuts_json_arr); array_index += 1)
    {
      cJSON* shortcut_json = cJSON_GetArrayItem(shortcuts_json_arr, array_index);
      if (!cJSON_IsObject(shortcut_json)) { goto json_parsing_end; }

      cJSON* combination_json = cJSON_GetObjectItemCaseSensitive(shortcut_json, "combination");      
      cJSON* command_json     = cJSON_GetObjectItemCaseSensitive(shortcut_json, "command");      
      if (!cJSON_IsString(combination_json)) { goto json_parsing_end; }
      if (!cJSON_IsString(command_json))     { goto json_parsing_end; }

      Str8 combination_str  = str8_from_cstr_copy((U8*)cJSON_GetStringValue(combination_json));
      Str8 command_name_str = str8_from_cstr_copy((U8*)cJSON_GetStringValue(command_json));

      Str8_list str_list = str8_split(scratch.arena, combination_str, Str8FromC("+"), 0);
      if (str_list.node_count != 2) { goto json_parsing_end; }

      Str8 modifier_key_str      = str_list.first->str;
      Str8 other_key_str         = str_list.last->str;
      Key modifier_key           = key_from_str8(modifier_key_str);
      Key other_key              = key_from_str8(other_key_str);
      OS_Event_modifier modifier = os_modifier_from_key(modifier_key);
      if (modifier == OS_Event_modifier__NONE || other_key == Key__NONE) { goto json_parsing_end; }

      if (P->key_combs_count < ArrayCount(P->key_combs))
      {
        B32 succ = add_key_combo(P, modifier, other_key, command_name_str);
        Handle(succ);
      }
    }

  } json_parsing_end:

  if (settings_json) { cJSON_Delete(settings_json); }

  end_scratch(&scratch);
}

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse)
{
  // Updating fading
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

  if (P->ui_state.is_widget_open) { return; }

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

  // Processing signals
  if (P->signals.terminate_app) { P->terminate_app = true; }
  P->signals = {};

  // Processing key combos
  for EachIndex(combo_index, P->key_combs_count)
  {
    Key_comb* combo = P->key_combs + combo_index;
    if (key_states[combo->key].went_down && key_states[key_from_os_event_mod(combo->mod)].is_down) 
    {
      Str8 command_name = str8_manual(combo->command_name_buffer, combo->command_name_buffer_count);
      run_command(P, command_name);
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

  if (!P->ui_state.is_widget_open)
  {
    if (P->current_mode == Pencil_mode__draw) { os_set_cursor(OS_Cursor__pen); }
    else if (P->current_mode == Pencil_mode__ruler) { os_set_cursor(OS_Cursor__crosshair); }
  }
}

///////////////////////////////////////////////////////////
// - Pencil state stuff
//
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

///////////////////////////////////////////////////////////
// - Shortcuts (Key combos)
//
B32 add_key_combo(Pencil_state* P, OS_Event_modifier mod, Key key, Str8 command_name)
{
  B32 succ = true;
  if (mod != OS_Event_modifier__control) { succ = false; }
  if (!(Key__a <= key && key <= Key__z)) { succ = false; }

  if (P->key_combs_count < ArrayCount(P->key_combs))
  {
    Handle(command_name.count < ArrayCount(Key_comb::command_name_buffer));
    Key_comb* new_comb = P->key_combs + (P->key_combs_count++);
    new_comb->key = key;
    new_comb->mod = mod;
    new_comb->command_name_buffer_count = (U8)command_name.count;
    memcpy(new_comb->command_name_buffer, command_name.data, command_name.count);
  } else { Handle(false); }

  return succ;
}

void run_command(Pencil_state* P, Str8 str)
{
  
  if (str8_match(str, command_names[Command_id__terminate_app], Str8_match__ignore_case)) { P->signals.terminate_app = true; }
  else if (str8_match(str, command_names[Command_id__open_command_line], Str8_match__ignore_case)) { P->ui_state.is_widget_open = true; }
  else { InvalidCodePath(); }
}

///////////////////////////////////////////////////////////
// - TODO: TEST UI HERE
//
void pencil_do_command_ui(Pencil_state* P, FP_Font font)
{
  os_set_cursor(OS_Cursor__arrow);

  Scratch scratch = get_scratch(0, 0);
  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos());
  ui_push_font(font);

  if (P->ui_state.is_widget_open)
  {
    for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__key && ev->key_event.went_down && ev->key_event.key == Key__escape)
      {
        P->ui_state.is_widget_open = false;
        os_consume_frame_event(ev);
        break;
      }
    }
    Str8 command_top_widget_id = Str8FromC("Command to widget id");
  
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
      Str8 entry_text = str8_manual(P->ui_state.text_entry_buffer, P->ui_state.text_entry_buffer_count);
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
    UI_Col() UI_Padded(ui_px(5))
    {
      ui_layout_x();
      UI_Parent(ui_box_make(command_top_widget_id, 0)) UI_Padded(ui_p_of_p(1, 0))
      {
        ui_width(ui_p_of_p(0.5, 1));
        ui_height(ui_p_of_p(0.5, 1));
        ui_b_color(P->ui_state.main_b_color);
        ui_layout_x();
        UI_Box* box = ui_box_make({}, UI_Box_flag__has_background);
        UI_Parent(box) UI_Padded(ui_px(10)) UI_Col() UI_Padded(ui_px(10))
        {
          Str8 text_entry_str = str8_manual(P->ui_state.text_entry_buffer, P->ui_state.text_entry_buffer_count);
    
          ui_width(ui_p_of_p(1, 0));
          ui_height(ui_fit());
          ui_border(1, white());
          UI_Parent(ui_box_make({}, UI_Box_flag__has_borders)) UI_PaddedAround(ui_px(5))
          {
            text_entry_ops = ui_text_edit_box(
              scratch.arena,
              true,
              ui_p_of_p(1, 0),
              P->ui_state.text_entry_buffer,
              P->ui_state.text_entry_buffer_count,
              ArrayCount(P->ui_state.text_entry_buffer),
              P->ui_state.text_entry_cursor,
              P->ui_state.text_entry_section,
              Str8FromC("Commands text entry box id")
            );     
          }
    
          ui_spacer(ui_px(5));
    
          ui_width(ui_p_of_p(1, 0));
          ui_height(ui_p_of_p(1, 0));
          UI_Row() 
          {
            // Clip box data we need
            Str8 clip_box_id          = Str8FromC("Command list clip box id");
            UI_Actions clip_box_acts  = ui_actions_from_id(clip_box_id);
            UI_Box_data clip_box_data = ui_box_data_from_box_id_prev_frame(clip_box_id);
    
            // Scroll bar data we need
            Str8 scroll_bar_id          = Str8FromC("Command list scroll bar id");
            UI_Actions scroll_bar_acts  = ui_actions_from_id(scroll_bar_id);
            UI_Box_data scroll_bar_data = ui_box_data_from_box_id_prev_frame(scroll_bar_id);
    
            // Data to set for this new ui build
            F32 prev_frame_clip_box_scroll_offfset = ui_box_id_get_prev_build_clip_offset(clip_box_id).y;
            F32 new_clip_offset_for_clip_box       = prev_frame_clip_box_scroll_offfset;
    
            // Clip box
            ui_width(ui_p_of_p(1, 0));
            ui_height(ui_p_of_p(1, 0));
            ui_border(1, white());
            ui_layout_x();
            UI_Box* clip_box = ui_box_make(clip_box_id, UI_Box_flag__clip|UI_Box_flag__has_borders);
            UI_Parent(clip_box) UI_PaddedAround(ui_px(5))
            {
              UI_Col()
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
    
                    ui_spacer(ui_px(3));
                    
                    UI_Actions entry_acts = ui_actions_from_box(command_entry_box);
                    if (entry_acts.is_hovered || P->ui_state.currently_chosen_command_index == command_index) { 
                      ui_set_b_color(command_entry_box, color_light_up(P->ui_state.main_b_color, 0.25)); 
                    }
                    if (entry_acts.is_clicked) { 
                      command_got_chosen   = true;
                      chosen_command_index = command_index; 
                    }
                  }
                }
              }
            
              // Adjusting the clip offset 
              if (clip_box_data.is_found)
              {
                if (P->ui_state.last_frame_navigated_commands_with_arrows)
                {
                  Str8 com_index_id             = str8_fmt(scratch.arena, "Command row entry id __ %lld", P->ui_state.currently_chosen_command_index);
                  UI_Box_data chosen_entry_data = ui_box_data_from_box_id_prev_frame(com_index_id);
                  if (chosen_entry_data.is_found)
                  {
                    RangeF32 visible_clip_box_on_screen_y_range = range_v2f32_y0y1(clip_box_data.on_screen_bbox);
                    RangeF32 entry_on_screen_y_range            = range_v2f32_y0y1(chosen_entry_data.on_screen_bbox);
                    if (!rangef32_contains_range(visible_clip_box_on_screen_y_range, entry_on_screen_y_range)) 
                    {
                      new_clip_offset_for_clip_box = -1.0f * rangef32_length(entry_on_screen_y_range) * P->ui_state.currently_chosen_command_index;
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
                      new_clip_offset_for_clip_box += ev->wheel_event.scroll_data * 10;
                      os_consume_frame_event(ev);
                      break;
                    }
                  }
                }
              }
            }
    
            // Figuring out if we need a scroll bar
            if (clip_box_data.is_found && clip_box_data.inner_content_dims.y > range_v2f32_dims(clip_box_data.on_screen_bbox).y)
            {
              ui_flags(UI_Box_flag__floating);
              ui_width(ui_p_of_p(1, 0));
              ui_height(ui_p_of_p(1, 0));
              ui_layout_x();
              UI_Row() UI_PaddedAround(ui_px(5))
              {
                ui_spacer(ui_p_of_p(1, 0));
    
                ui_width(ui_px(15));
                ui_height(ui_p_of_p(1, 0));
                UI_Box* scroll_bar_box = ui_box_make(scroll_bar_id, 0);
                UI_Parent(scroll_bar_box) 
                {
                  // TODO: Scroll bar here
                  // [ ] - Have it move around the screen
                  // [ ] - Spawn it first, update the data for it later
                  // [ ] - Handle thumb here as well
      
                  Str8 thumb_box_id    = Str8FromC("Scroll bar thumb button id");
                  F32 thumb_offset     = 0.0f;
                  F32 thumb_size       = 0.0f;
                  F32 max_thumb_offset = 0.0f;
                  F32 max_vp_offset    = 0.0f;
      
                  if (scroll_bar_data.is_found)
                  {
                    Assert(clip_box_data.is_found);
      
                    F32 vp              = range_v2f32_dims(clip_box_data.on_screen_bbox).y;
                    F32 content_size    = clip_box_data.inner_content_dims.y;
                    F32 scroll_bar_size = range_v2f32_dims(scroll_bar_data.on_screen_bbox).y;
                    
                    thumb_size      = (vp / content_size) * scroll_bar_size;
                    if (thumb_size < 30.0f) { thumb_size = 30.0f; }
      
                    max_thumb_offset  = scroll_bar_size - thumb_size;
                    max_vp_offset     = content_size - vp;
      
                    thumb_offset = (-1.0f * prev_frame_clip_box_scroll_offfset / max_vp_offset) * max_thumb_offset;
                  
                    OutputDebugStringF("Thumb offset: %f \n", thumb_offset);
                  }
      
                  if (scroll_bar_acts.is_down)
                  {
                    // TODO: This doesnt work here since only 1 element per ui might be down
                    //       since this is float, this here is on top of othe boxed, 
                    //       specifically the command entry boxes inside clip, 
                    //       those are build first and therefore when i go down,
                    //       those become the interacted_with once. This is not a bug, 
                    //       but rather how it works, thought i have no idea
                    //       what to do about it.
                    BP;
                    F32 scrolL_bar_relative_pos = scroll_bar_data.on_screen_bbox.min.y - ui_get_mouse_y();
                    F32 new_thumb_offset = scrolL_bar_relative_pos;
                    if (max_thumb_offset != 0.0f && max_vp_offset != 0.0f) 
                    {
                      new_clip_offset_for_clip_box = -1.0f * (new_thumb_offset / max_thumb_offset) * max_vp_offset;
                    }
                  }
      
                  ui_height(ui_p_of_p(1, 0));
                  ui_width(ui_p_of_p(1, 0));
                  UI_Col()
                  {
                    ui_spacer(ui_px(thumb_offset));
      
                    // todo: There is a bug here with blending, its like there is alpha blend but not fully correct
                    ui_width(ui_p_of_p(1, 0));
                    ui_height(ui_px(thumb_size));
                    ui_b_color(v4f32(1.0f, 1.0f, 1.0f, 0.15f));
                    UI_Box* thumb_box = ui_box_make(thumb_box_id, UI_Box_flag__has_background);
                    
                    UI_Actions thumb_acts = ui_actions_from_box(thumb_box);
                    if (thumb_acts.is_hovered || scroll_bar_acts.is_hovered) { ui_set_b_color(thumb_box, v4f32(1.0f, 1.0f, 1.0f, 0.35f)); };            
                  }
      
                  
                }
              }
            }
    
            // Fixing and setting the new clip offset to the clip box
            {
              F32 space_after_vp = clip_box_data.inner_content_dims.y - range_v2f32_dims(clip_box_data.on_screen_bbox).y;
              if (space_after_vp < 0.0f) { space_after_vp = 0.0f; }
      
              if (new_clip_offset_for_clip_box > 0.0f) { 
                new_clip_offset_for_clip_box = 0.0f; 
              }
              else if (new_clip_offset_for_clip_box < -space_after_vp) { 
                new_clip_offset_for_clip_box = -space_after_vp; 
              }
              ui_box_set_clip_offset_y(clip_box, new_clip_offset_for_clip_box);
            }
    
          }
        }
      }
    }
    // Resseting
    P->ui_state.last_frame_navigated_commands_with_arrows = false;
  
    // Updating data 
    {
      ui_aply_text_ops(
        text_entry_ops, 
        P->ui_state.text_entry_buffer, 
        ArrayCount(P->ui_state.text_entry_buffer), 
        &P->ui_state.text_entry_buffer_count, 
        &P->ui_state.text_entry_cursor, 
        &P->ui_state.text_entry_section, 
        0, 0 
      );
  
      // Cycling over the the filtered commands
      for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
      {
        if (ev->kind == OS_Event_kind__key && (ev->key_event.went_down || ev->key_event.repeat_down) && ev->key_event.key == Key__arrow_up)
        {
          P->ui_state.last_frame_navigated_commands_with_arrows = true;
          for EachIndex(i, ArrayCount(filtered_command_data))
          {
            P->ui_state.currently_chosen_command_index -= 1;
            clamp_u64_inplace(&P->ui_state.currently_chosen_command_index, 0, ArrayCount(command_names) - 1);
            if (filtered_command_data[P->ui_state.currently_chosen_command_index].is_filtered_in) {
              break;
            }
          }
          os_consume_frame_event(ev);
        }
        else
        if (ev->kind == OS_Event_kind__key && (ev->key_event.went_down || ev->key_event.repeat_down) && ev->key_event.key == Key__arrow_down)
        {
          P->ui_state.last_frame_navigated_commands_with_arrows = true;
          for EachIndex(i, ArrayCount(filtered_command_data))
          {
            P->ui_state.currently_chosen_command_index += 1;
            if (P->ui_state.currently_chosen_command_index == ArrayCount(command_names)) { P->ui_state.currently_chosen_command_index = 0; }
            if (filtered_command_data[P->ui_state.currently_chosen_command_index].is_filtered_in) {
              break;
            }
          }
          os_consume_frame_event(ev);
        }
        else
        if (ev->kind == OS_Event_kind__key && ev->key_event.went_down && ev->key_event.key == Key__enter)
        {
          command_got_chosen = true;
          chosen_command_index = P->ui_state.currently_chosen_command_index;
        }
      }
  
      if (command_got_chosen)
      {
        Str8 command_name = command_names[chosen_command_index];
        run_command(P, command_name);
        P->ui_state.is_widget_open = false;
      }
    }
  }

  ui_pop_font();
  ui_end_build();
  end_scratch(&scratch);
}


#endif