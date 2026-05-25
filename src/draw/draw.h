#ifndef DRAW_API_H
#define DRAW_API_H

#include "render/render.h"
#include "font_provider/font_provider.h"

enum D_Command_type : U32 {
  D_Command_type__Rect,
  D_Command_type__Texture,
  D_Command_type__Line,
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
  V4F32 tint;
  Rect dest_rect;
  Rect src_rect;
};

struct D_Line_command {
  V4F32 color;
};

struct D_Command {
  union {
    D_Rect_command rect_c;
    D_Texture_command texture_c;
    D_Line_command line_c;
  } u;
};

struct D_Command_node {
  D_Command command;
  D_Command_node* next;
};

struct D_Command_batch {
  // Provided by the caller to the batch maker
  D_Command_type command_type;                 
  R_Target       texture; // Right now this might or might not be used, kind of like for a fat struct

  // Provided by batch setting stacks
  R_Target     target;                       
  Rect         scissor_rect;                 
  R_Blend_kind blend_kind;                   
  R_Fill_mode  fill_mode;                       
                                                  
  D_Command_node* first_command_node;           
  D_Command_node* last_command_node;            
  U64             count;                        
                                                  
  D_Command_batch* next_batch; 
};

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
    R_Blend_kind blend_kind;
    R_Target     render_target;
    Rect         scissor_rect;
    R_Fill_mode  fill_mode;
  } default_batch_settings;
  //
  R_Blend_kind arr_of_blend_kinds[64];
  U64 current_blend_kind_count;
  //
  R_Target arr_of_render_targets[64];
  U64 current_render_target_count;
  //
  Rect arr_of_scissor_rects[64];
  U64 current_scissor_rect_count;
  // 
  R_Fill_mode arr_of_fill_modes[64];
  U64 current_fill_mode_count;
};

// - State
D_State* d_get_state();
void     d_init();
void     d_release();

// - Batching
void                  d_begin_batching(R_Target target) ;
void                  d_end_batching();
D_Command_batch_list* d_get_batch_list();
D_Command_batch*      d_add_new_batch(D_Command_type command_type, R_Target texture);
D_Command_batch*      d_get_or_add_batch_for_settings(D_Command_type command_type, R_Target texture);
void                  d_add_command_to_batch(D_Command_batch* batch, D_Command command);

// - Low level draw commands that require the caller to know how the shader works
void d_add_rect_command(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness, V4F32 border_color);
void d_add_texture_command(R_Target texture, Rect dest_rect, Rect src_rect, V4F32 tint);
void d_add_line_command(V4F32 color);

// - Higher level draw commands that dont require the caller to know how the shader works
void d_draw_rect(Rect rect, V4F32 color);
void d_draw_rect_pro(Rect rect, V4F32 color_x0y0, V4F32 color_x1y0, V4F32 color_x0y1, V4F32 color_x1y1, V4F32 corner_radii, F32 softness);

void d_draw_rect_inset_borders(Rect rect, V4F32 color, F32 thickness, V4F32 corner_radii, F32 softness);

void d_draw_circle(V2F32 center, F32 r, V4F32 color, F32 softness);
void d_draw_circle_inset_border(V2F32 center, F32 r, V4F32 color, F32 thickness, F32 softness);

void d_draw_texture(R_Target texture, V2F32 pos);
void d_draw_texture_pro(R_Target texture, Rect dest_rect, Rect source_rect, V4F32 tint);

void d_draw_text(Str8 text, FP_Font font, V2F32 pos, V4F32 color);
void d_draw_text_f(const char* fmt, FP_Font font, V2F32 pos, V4F32 color, ...);

// - Push/Pops 
void         d_push_blend_kind(R_Blend_kind blend_kind);
void         d_pop_blend_kind();
R_Blend_kind __d_get_current_blend_kind__defaults();
#define      D_Blend(blend_kind) DeferLoop(d_push_blend_kind(blend_kind), d_pop_blend_kind())

void     d_push_render_target(R_Target target);
void     d_pop_render_target();
R_Target __d_get_current_render_target__defaults();
#define  D_RenderTarget(target) DeferLoop(d_push_render_target(target), d_pop_render_target())

void    d_push_scissor_rect(Rect rect);
void    d_pop_scissor_rect();
Rect    __d_get_current_scissor_rect__defaults();
#define D_ScissorRect(rect) DeferLoop(d_push_scissor_rect(rect), d_pop_scissor_rect())

void        d_push_fill_mode(R_Fill_mode fill_mode);
void        d_pop_fill_mode();
R_Fill_mode __d_get_current_fill_mode__defaults();
#define D_FillMode(fill_mode) DeferLoop(d_push_fill_mode(fill_mode), d_pop_fill_mode())


#endif