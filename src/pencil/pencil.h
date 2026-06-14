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
  // Pencil_mode__temp_texture,
};

struct Draw_record {
  // These are allocated when done 
  R_Target drawing_texture;

  // Fat struct data
  B32 is_delete_texture;
  B32 is_eraser_texture;
  B32 is_fading_texture;
  F32 time_left_till_full_fade_sec;

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

// note: These are to be sorted
#define COMMAND_DATA_X_LIST                           \
  X(swap_to_draw,     Str8FromC("Swap to draw")     ) \
  X(swap_to_eraser,   Str8FromC("Swap to eraser")   ) \
  X(swap_to_ruler,    Str8FromC("Swap to ruller")   ) \
  X(terminate_app,    Str8FromC("Terminate app")    ) \
  X(toggle_line_fade, Str8FromC("Toggle line fade") ) \
  X(make_background_blue, Str8FromC("Make background blue") ) \
  X(__Fake_COMMAND_1, Str8FromC("__Fake_COMMAND_1")) \
  X(__Fake_COMMAND_2, Str8FromC("__Fake_COMMAND_2")) \
  X(__Fake_COMMAND_3, Str8FromC("__Fake_COMMAND_3")) \
  X(__Fake_COMMAND_4, Str8FromC("__Fake_COMMAND_4")) \
  X(__Fake_COMMAND_5, Str8FromC("__Fake_COMMAND_5"))  \
  X(__Fake_COMMAND_511, Str8FromC("__Fake_COMMAND_5")) \
  X(__Fake_COMMAND_51, Str8FromC("__Fake_COMMAND_5")) \
  X(__Fake_COMMAND_52, Str8FromC("__Fake_COMMAND_5")) \
  X(__Fake_COMMAND_33, Str8FromC("__Fake_COMMAND_5")) \
  X(__Fake_COMMAND_54, Str8FromC("__Fake_COMMAND_5")) \
  X(__Fake_COMMAND_55, Str8FromC("__Fake_COMMAND_5"))

enum Command_id : U32 {
  // Command_id__NONE,
  #define X(id, str_name) Command_id__##id,
    COMMAND_DATA_X_LIST
  #undef X
  Command_id__COUNT,
};

Str8 command_names[Command_id__COUNT] = {
  #define X(id, str_name) str_name,
    COMMAND_DATA_X_LIST
  #undef X
};

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
  // R_Target draw_texture_always_fresh; // todo: Rename this to be a less complicated name since now we only have 1 of them (no non_fresh_texture)
  R_Target current_initial_draw_texture; // This is the state of the texture that was before every draw in the current draw records pool happened 

  F32 fading_texture_fade_time;

  // Pool of draw records
  #define DRAW_RECORDS_MAX_COUNT 50
  Draw_record pool_of_draw_records[DRAW_RECORDS_MAX_COUNT];
  Draw_record* first_free_draw_record;
  Draw_record* last_free_draw_record;
  
  Draw_record* first_record;
  Draw_record* last_record;
  Draw_record* current_record; // This might be zero. TH

  B32 is_mid_drawing;
  B32 is_erasing_mode;
  B32 is_erasing_mode_for_a_single_drawing; // todo: This no longer works
  B32 is_make_new_texture_fading;

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
  B32 signal_make_b_blue;
  //
  B32 signal_swap_to_pen;
  //
  B32 signal_new_pen_color_hsva;
  V4F32 new_pen_color_hsva;
  // 
  B32 signal_swap_to_ruler;
  // 
  B32 signal_swap_to_draw;
  // 
  B32 signal_toggle_line_fade;

  struct {
    // TODO: Have this be false when release
    B32 is_widget_open = true;

    U8 text_entry_buffer[64];
    U64 text_entry_buffer_count;
    U64 text_entry_cursor;
    U64 text_entry_section;

    U64 currently_chosen_command_index;
    B32 last_frame_navigated_commands_with_arrows;

    V4F32 main_b_color = rgba_from_hex(0x695A09FF);
  } ui_state;

  // Misc
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
void delete_draw_record__invalidates_record(Pencil_state* P, Draw_record* record_to_delete);
void add_shortcut(Pencil_state* P, OS_Event_modifier mod, Key key, Str8 command_name);
void run_command_from_name(Pencil_state* P, Str8 command_name);
void command_terminate_app(Pencil_state* P);
void command_swap_to_ruller(Pencil_state* P);
void command_swap_to_draw(Pencil_state* P);
void command_toggle_line_fade(Pencil_state* P);
void command_swap_to_eraser(Pencil_state* P);
void command_make_background_blue(Pencil_state* P);
void command_open_command_list(Pencil_state* P);
B32 is_valid_command_name(Str8 command_name);
//
//
#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P);
#endif

#endif