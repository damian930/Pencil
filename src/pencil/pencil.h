#ifndef PENCIL_H
#define PENCIL_H

#include "__third_party/cjson/cJSON.h"

#include "core/core_include.h"
#include "os/win32.h"
#include "render/render.h"

const U32 MAX_PEN_SIZE = 100;
const U32 MIN_PEN_SIZE = 1;

enum Pencil_mode : U32 {
  Pencil_mode__draw,
  Pencil_mode__ruler,
  Pencil_mode__temp_texture,
};

struct Draw_record {
  // These are allocated when done 
  R_Target texture_before_we_affected;
  R_Target texture_after_we_affected;

  // These are used in general to have a list of drawings 
  // and are also used for managing a free list 
  Draw_record* next;
  Draw_record* prev;
};

// note: I be forgetting why i have an optional here. Its not for cases where the textures might not ge created, but
//       rather for cases when the state of the Pencil might not be legal for creation of new draw records.
struct Draw_record_registration_result {
  B32 succ;
  Draw_record* record;  
};

#define COMMAND_NAME_TERMINATE_APP  Str8FromC("Terminate app")
#define COMMAND_NAME_SWAP_TO_RULER  Str8FromC("Swap to ruller")
#define COMMAND_NAME_SWAP_TO_DRAW   Str8FromC("Swap to draw")

struct Shortcut_chord {
  OS_Event_modifier mod;
  Key key;
  U8 command_name_buffer[32];
  U8 command_name_buffer_count;
};

struct Pencil_state {
  Arena* frame_arena;
  
  Pencil_mode current_mode;

  U32 pen_size;
  V4F32 pen_color_hsva;
  U32 eraser_size;

  U32 draw_texures_width;
  U32 draw_texures_height;
  R_Target draw_texture_always_fresh; // todo: Rename this to be a less complicated name since now we only have 1 of them (no non_fresh_texture)

  // Testing this for now
  R_Target temp_drawing_texture;
  F32 temp_texture_initial_time_to_fade;
  F32 temp_texture_time_left_to_fade;

  // Pool of draw records
  #define DRAW_RECORDS_MAX_COUNT 50
  Draw_record pool_of_draw_records[DRAW_RECORDS_MAX_COUNT];
  Draw_record* first_free_draw_record;
  Draw_record* last_free_draw_record;
  
  Draw_record* first_record;
  Draw_record* last_record;
  Draw_record* current_record; // This might be zero, i dont know if i dislike it yet

  B32 is_mid_drawing;
  B32 is_erasing_mode;
  B32 is_erasing_mode_for_a_single_drawing;

  // Some state for the rulling mode to go over
  B32 is_mid_ruling;
  V2F32 ruling_start_pos;
  V2F32 ruling_end_pos;

  #define MAX_CHORD_COUNT 50
  Shortcut_chord chords[MAX_CHORD_COUNT];
  U64 chord_count;
  //
  B32 terminate_app;

  // Signals 
  //
  B32 signal_new_pen_size;
  U32 new_pen_size;
  //
  B32 signal_new_eraser_size;
  U32 new_eraser_size;
  // 
  B32 signal_swap_to_eraser;
  //
  B32 signal_swap_to_pen;
  //
  B32 signal_new_pen_color_hsva;
  V4F32 new_pen_color_hsva;
  // 
  B32 signal_swap_to_ruler;
  // 
  B32 signal_swap_to_draw;

  // Misc
  // Font font_texture_for_ui;
  // V2U64 last_screen_dims;
  B32 show_brush_ui_menu;
};

// - Main passes
void pencil_init(Pencil_state* P);
void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse);
void pencil_render(const Pencil_state* P);
void pencil_do_ui(Pencil_state* P, FP_Font font);

// - Other
Draw_record_registration_result register_new_draw_record(Pencil_state* P);
Draw_record* __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(Pencil_state* P);
void add_shortcut(Pencil_state* P, OS_Event_modifier mod, Key key, Str8 command_name);
void run_command_from_name(Pencil_state* P, Str8 command_name);
void command_terminate_app(Pencil_state* P);
void command_swap_to_ruller(Pencil_state* P);
void command_swap_to_draw(Pencil_state* P);
//
//
#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P);
#endif

#endif