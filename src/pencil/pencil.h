#ifndef PENCIL_H
#define PENCIL_H

#include "core/core_include.h"
#include "os/win32.h"

struct Draw_record {
  // These are allocated when done 
  ID3D11Texture2D*        texture_before_we_affected;
  ID3D11Texture2D*        texture_after_we_affected;
  ID3D11RenderTargetView* texture_before_we_affected_rtv;  
  ID3D11RenderTargetView* texture_after_we_affected_rtv;   

  // These are used in general to have a list of drawings 
  // and are also used for managing a free list 
  Draw_record* next;
  Draw_record* prev;
};

struct Draw_record_registration_result {
  B32 succ;
  Draw_record* record;  
};

struct Pencil_state {
  Arena* frame_arena;
  
  U32 pen_size;
  V4F32 pen_color_hsva;
  U32 eraser_size;

  U32 draw_texures_width;
  U32 draw_texures_height;
  // todo: Rename this to be a less complicated name since now we only have 1 of them (no non_fresh_texture)
  ID3D11Texture2D*        draw_texture_always_fresh;
  ID3D11RenderTargetView* draw_texture_always_fresh_rtv; 

  // Pool of draw records
  #define DRAW_RECORDS_MAX_COUNT 50
  Draw_record pool_of_draw_records[DRAW_RECORDS_MAX_COUNT];
  U64 count_of_pool_draw_records_in_use; // This inсludes if they are in the free list 
  Draw_record* first_free_draw_record;
  Draw_record* last_free_draw_record;
  
  // todo: I have that current record might be 0 sometimes and we have to check for it or we crash, at least dont crash
  Draw_record* first_record;
  Draw_record* last_record;
  Draw_record* current_record;

  B32 is_mid_drawing;
  
  B32 is_erasing_mode;

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

  // Misc
  // Font font_texture_for_ui;
  // V2U64 last_screen_dims;
  B32 show_brush_ui_menu;
};

// - Main passes
void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse);
void pencil_render(const Pencil_state* P);
void pencil_do_ui(Pencil_state* P, FP_Font font);

// - Other
Draw_record_registration_result register_new_draw_record(Pencil_state* P);
Draw_record* __get_new_draw_record_from_pool__nullable__private_for__register_new_draw_record(Pencil_state* P);
//
#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P);
#endif

#endif