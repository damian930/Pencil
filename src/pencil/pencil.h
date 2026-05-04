#ifndef PENCIL_H
#define PENCIL_H

#include "core/core_include.h"
#include "os/win32.h"

struct Draw_record {
  ID3D11RenderTargetView* texture_before_we_affected; // This is allocated when done drawing
  ID3D11RenderTargetView* texture_after_we_affected;  // This is allocated when done drawing

  // These are also used for draw record free list
  Draw_record* next;
  Draw_record* prev;
};

struct Pencil_state {
  Arena* arena;
  Arena* frame_arena;
  
  U32 pen_size;
  V4F32 pen_color;
  U32 eraser_size;

  U32 draw_texures_width;
  U32 draw_texures_height;
  ID3D11RenderTargetView* draw_texture_always_fresh; 
  ID3D11RenderTargetView* draw_texture_not_that_fresh;

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

  // Stuff for while drawing
  B32 is_mid_drawing;
  B32 is_erasing_mode;

  // Signals (These are just here like this right now)
  // B32 signal_new_pen_size;
  // U64 new_pen_size;
  //
  // B32 signal_new_eraser_size;
  // U64 new_eraser_size;
  // 
  // B32 signal_swap_to_eraser;
  // B32 signal_swap_to_pen;

  // Misc
  // Font font_texture_for_ui;
  // V2U64 last_screen_dims;
  // B32 show_brush_ui_menu;
  // Str8 brush_menu_ui_id;

  // Stuff for drawing that i dont yet have as a separate thing
  D3D_Program texture_to_screen_program;
};

struct Draw_record_registration_result {
  B32 succ;
  Draw_record* record;  
};

void draw_rect(
  const Pencil_state* P,
  D3D_State* d3d, ID3D11RenderTargetView* render_target,  
  F32 x, F32 y, F32 width, F32 height, V4U8 color
);

Draw_record* get_new_draw_record_from_pool__nullable(Pencil_state* P);
Draw_record_registration_result register_new_draw_record(Pencil_state* P, D3D_State* d3d, B32 is_ui_capturing_mouse);
void copy_from_texture_to_texture(D3D_State* d3d, ID3D11RenderTargetView* dest_rtv, ID3D11RenderTargetView* src_rtv);

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse, D3D_State* d3d);
void pencil_render(const Pencil_state* P, D3D_State* d3d);

#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P, D3D_State* d3d);
#endif

#endif