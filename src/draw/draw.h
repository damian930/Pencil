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

  // Fat stuct data
  ID3D11Texture2D* texture;
  
  D_Command_node* first_command_node;
  D_Command_node* last_command_node;
  U64 count;

  D_Command_batch* next_batch; 
};  

struct D_Command_batch_list {
  D_Command_batch* first;
  D_Command_batch* last;
  U64 count;
};

struct __D_Rect_builder {
  Rect rect;
  V4F32 corner_colors[UV__COUNT];

  __D_Rect_builder* (*color) (V4F32 colors);

  void (*add) ();
  D_Command (*get) ();
};

struct D_State {
  Arena* arena_for_draw_commands;
  D_Command_batch_list command_batch_list;
 
  // Some state, nothing specific yet
  __D_Rect_builder rect_builder;
  ID3D11RenderTargetView* current_rtv;
  Rect current_scissor_rect;
};


// - Internal builder functions
__D_Rect_builder* __d_builder_func_color(V4F32 color);
D_Command         __d_builder_func_get();
void              __d_builder_func_add();

// - State
D_State* d_get_state();
void d_init();
void d_release();

// - Batching
void                  d_begin_batching();
void                  d_end_batching();
D_Command_batch_list* d_get_batch_list();
D_Command_batch*      d_add_new_batch(D_Command_type command_type, ID3D11Texture2D* texture);
D_Command_batch*      d_get_or_add_batch_for_settings(D_Command_type command_type, ID3D11Texture2D* texture);
void                  d_add_command_to_batch(D_Command_batch* batch, D_Command command);

// - Draw commands
__D_Rect_builder* d_draw_rect(Rect rect);
void d_add_rect_command_ex(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness);
void d_add_rect_command(Rect rect, V4F32 color);
void d_add_texture_command(ID3D11Texture2D* texture, Rect dest_rect, Rect src_rect, B32 is_text, V4F32 text_color);

// - Misc
void d_set_render_target(ID3D11RenderTargetView* rtv);
void d_set_scissor_rect(Rect rect);

#endif