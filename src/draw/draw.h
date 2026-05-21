#ifndef DRAW_API_H
#define DRAW_API_H

#include "render/render.h"

enum D_Command_type : U32 {
  D_Command_type__Rect,
  D_Command_type__Texture,
};

struct D_Rect_command {
  Rect rect;
  V4F32 vertex_color[UV__COUNT];
  F32 corner_radius[UV__COUNT];
  V4F32 border_color;
  F32 border_thickness;
  F32 softness;
};

struct D_Texture_command {
  B32 is_text;
  V4F32 text_color;
  Rect dest_rect;
  Rect src_rect;
};

struct D_Command {
  union {
    D_Rect_command rect_c;
    D_Texture_command texture_c;
  } u;
};

struct D_Command_node {
  D_Command command;
  D_Command_node* next;
};

struct D_Command_batch {
  D_Command_type command_type;
  ID3D11RenderTargetView* rtv;
  Rect scissor_rect;
  D3D_Blend_kind blend_kind;

  // Fat stuct data
  ID3D11Texture2D* texture;

  D_Command_node* first_command_node;
  D_Command_node* last_command_node;
  U64 count;

  D_Command_batch* next_batch; 
};  
#define __D_COMMAND_BATCH_SIZE_TEST 80

struct D_Command_batch_list {
  D_Command_batch* first;
  D_Command_batch* last;
  U64 count;
};

struct D_State {
  Arena* arena_for_draw_commands;
  D_Command_batch_list command_batch_list;
 
  // Batch setting stacks
  struct {
    D3D_Blend_kind blend_kind;
    ID3D11RenderTargetView* render_target;
    Rect scissor_rect;
  } defaults;
  //
  D3D_Blend_kind arr_of_blend_kinds[64];
  U64 current_blend_kind_count;
  //
  ID3D11RenderTargetView* arr_of_render_targets[64];
  U64 current_render_target_count;
  //
  Rect arr_of_scissor_rects[64];
  U64 current_scissor_rect_count;
};

// - State
D_State* d_get_state();
void     d_init();
void     d_release();

// - Batching
void d_begin_batching(R_Target target) ;
void                  d_end_batching();
D_Command_batch_list* d_get_batch_list();
D_Command_batch*      d_add_new_batch(D_Command_type command_type, ID3D11Texture2D* texture);
D_Command_batch*      d_get_or_add_batch_for_settings(D_Command_type command_type, ID3D11Texture2D* texture);
void                  d_add_command_to_batch(D_Command_batch* batch, D_Command command);

// - Push/Pops 
void           d_push_blend_kind(D3D_Blend_kind blend_kind);
void           d_pop_blend_kind();
D3D_Blend_kind __d_get_current_blend_kind__defaults();
#define        D_Blend(blend_kind) DeferLoop(d_push_blend_kind(blend_kind), d_pop_blend_kind())

void                    d_push_render_target(ID3D11RenderTargetView* rtv);
void                    d_pop_render_target();
ID3D11RenderTargetView* __d_get_current_render_target__defaults();
#define                 D_RenderTarget(target) DeferLoop(d_push_render_target(target), d_pop_render_target())

void    d_push_scissor_rect(Rect rect);
void    d_pop_scissor_rect();
Rect    __d_get_current_scissor_rect__default();
#define D_ScissorRect(rect) DeferLoop(d_push_scissor_rect(rect), d_pop_scissor_rect())

// - Low level draw commands that require the caller to know how the shader works
void d_add_rect_command(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness, V4F32 border_color);
void d_add_texture_command(ID3D11Texture2D* texture, Rect dest_rect, Rect src_rect, B32 is_text, V4F32 text_color);

// - Higher level draw commands that dont require the caller to know how the shader works
void d_draw_rect(Rect rect, V4F32 color);
void d_draw_rect_pro(Rect rect, V4F32 color_x0y0, V4F32 color_x1y0, V4F32 color_x0y1, V4F32 color_x1y1, V4F32 corner_radii, F32 softness);

void d_draw_rect_inset_borders(Rect rect, V4F32 color, F32 thickness, V4F32 corner_radii, F32 softness);

void d_draw_circle(V2F32 center, F32 r, V4F32 color, F32 softness);
void d_draw_circle_inset_border(V2F32 center, F32 r, V4F32 color, F32 thickness, F32 softness);

#endif