#ifndef RENDERER_D3D11_H
#define RENDERER_D3D11_H

// D3D 
#include "d3d11.h"
#include "dxgi.h"
#include "dxgidebug.h"
#include "dxgi1_3.h"
#include "d3dcompiler.h"

// DWM
#include "dwmapi.h"
#include "dcomp.h"

#include "__third_party/stb/stb_image.h"

#include "core/core_include.h"

// Misc structs here
// todo:
// note: I wanted to maybe make this be just a zero id for texture like in opengl, 
//       but since this is a pointer I cant just return a 0 pointer
//       or have a struct to reference as 0, since you cant make an instance of this type other than a 
//       pointer to it.
struct D3D_Texture_result {
  B32 succ;  
  ID3D11Texture2D* texture;
};

struct Image {
  U8* data;
  U64 width_in_px;
  U64 height_in_px;
  U64 bytes_per_pixel;
  // U64 row_stride; // There are no images right now that might have extra padding
};

// ========
// ========
struct D3D_Rect_instance_data {
  V4F32 color_00;
  V4F32 color_10;
  V4F32 color_01;
  V4F32 color_11;
  
  F32 origin_x; 
  F32 origin_y; 

  F32 width;
  F32 height;

  F32 corner_radius_00;
  F32 corner_radius_10;
  F32 corner_radius_01;
  F32 corner_radius_11;

  // F32 _padding_[3];
};

struct D3D_Rect_unifrom_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};  
// ========
// ========

struct D3D_Texture_instance_data {
  V4F32 text_color;

  V2F32 dest_rect_origin;
  V2F32 dest_rect_size;
  
  V2F32 src_rect_origin;
  V2F32 src_rect_size;
  
  V2F32 src_texture_dims;

  B32 is_text_texture;

  F32 _padding_[2];
};

struct D3D_Texture_uniform_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};
// ========
// ========

struct D3D_Program {
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11InputLayout* input_layout;
};

struct D3D_State {
  // These we get at initialisation
  //
  // State
  IDXGIFactory2*       dxgi_factory;
  IDXGIAdapter*        dxgi_adapter;
  ID3D11Device*        device;
  ID3D11DeviceContext* context;
  // 
  IDXGISwapChain1*        swap_chain;
  ID3D11RasterizerState*  rasterizer_state;
  ID3D11BlendState*       alpha_blend_state;
  ID3D11SamplerState*     sampler;
  //
  ID3D11Texture2D*        frame_buffer_texture;
  ID3D11RenderTargetView* frame_buffer_rtv;
  
  // Draw programs
  ID3D11Buffer* rect_program_ia_buffer;
  ID3D11Buffer* rect_program_uniform_buffer;
  D3D_Program   rect_program;
  //
  ID3D11Buffer* texture_program_ia_buffer;
  ID3D11Buffer* texture_program_uniform_buffer;
  D3D_Program   texture_program;

  // -----------------------------
  // Debug stuff for layer
  // Arena* debug_arena;
  // Str8* error_messages_arr;
  // U64 error_messages_count;
};

extern global D3D_State __d3d_g_state;

// - State
D3D_State* r_get_state();
void r_init();
void r_relesase();

// - Render pass
void r_render_begin(V2F32 vp_dims);
void r_render_end();

// - Drawing
void r_clear_frame_buffer(V4F32 color);
void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color);
//
void r_draw_rect_pro(Rect rect, V4F32 rect_color, F32 border_line_thickness, V4F32 border_color);
void r_draw_circle(ID3D11RenderTargetView* rtv, F32 center_x, F32 center_y, F32 radius, V4F32 color, B32 turn_off_blend_for_this);
void r_draw_texture(ID3D11RenderTargetView* dest_rtv, Rect rect_in_dest, ID3D11RenderTargetView* src_rtv, Rect rect_in_src);

// struct FP_Font; // Dont just include fp in here, since fp need the renderer, so we got a circular dependancy
// void r_draw_text(ID3D11RenderTargetView* dest_rtv, Str8 text, V2F32 pos, FP_Font font, V4F32 color);

// - Other
ID3D11RenderTargetView* r_get_frame_buffer_rtv();
ID3D11RenderTargetView* r_make_texture(U32 width, U32 height);
D3D_Texture_result r_texture_from_rtv(ID3D11RenderTargetView* rtv);
V2F32 r_get_texture_dims(ID3D11Texture2D* rtv);
D3D_Program r_program_from_file(const WCHAR* shader_program_file, 
                                const char* v_shader_main_f_name, 
                                const char* p_shader_main_f_name, 
                                const D3D11_INPUT_ELEMENT_DESC* opt_desc_arr,
                                U32 desc_arr_count);

// - Misc
Image r_image_from_texture(Arena* arena, ID3D11RenderTargetView* rtv);
void r_export_texture(ID3D11RenderTargetView* rtv, Str8 file_path);
ID3D11Texture2D* r_load_texture_from_file(Str8 file_name);
ID3D11Texture2D* r_load_texture_from_image(Image image);
void r_copy_from_texture_to_texture(ID3D11RenderTargetView* dest_rtv, ID3D11RenderTargetView* src_rtv);
void r_scissoring_set(Rect rect);
void r_scissoring_clear();

///////////////////////////////////////////////////////////
// - Batching (for now here)
//
enum D_Command_type : U32 {
  D_Command_type__Rect,
  D_Command_type__Texture,
};

struct D_Rect_command {
  Rect rect;
  V4F32 vertex_color[UV__COUNT];
  F32 corner_radius[UV__COUNT];
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

struct D_State {
  Arena* arena_for_draw_commands;
  D_Command_batch_list command_batch_list;
};

global D_State __d_g_state = {};

D_State* d_get_state() { return &__d_g_state; }

void d_init()
{
  __d_g_state.arena_for_draw_commands = arena_alloc(Gigabytes(1));
}
void d_release() { /*todo:*/ }

void d_begin_batching() 
{ 
  D_State* draw_state = d_get_state();
  
  draw_state->command_batch_list = {};
  arena_clear(draw_state->arena_for_draw_commands); 
}
void d_end_batching() { /*Nothing here*/ }

D_Command_batch_list* d_get_batch_list() { return &d_get_state()->command_batch_list; }

D_Command_batch* d_add_new_batch(D_Command_type command_type, ID3D11Texture2D* texture)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_batch* new_batch = ArenaPush(arena, D_Command_batch);
  new_batch->command_type = command_type;
  new_batch->texture      = texture;
  
  QueuePushBack_Name(&draw_state->command_batch_list, new_batch, first, last, next_batch);
  draw_state->command_batch_list.count += 1;

  return new_batch;
}

void d_add_command_to_batch(D_Command_batch* batch, D_Command command)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_node* node = ArenaPush(arena, D_Command_node);
  node->command = command;
  
  QueuePushBack_Name(batch, node, first_command_node, last_command_node, next);
  batch->count += 1;
}

void d_add_rect_command_ex(Rect rect, V4F32 corner_color[UV__COUNT], V4F32 corner_radiuses)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_batch* batch = draw_state->command_batch_list.last;
  if (batch == 0 || batch->command_type != D_Command_type__Rect)
  {
    batch = d_add_new_batch(D_Command_type__Rect, Null);
  }

  D_Command command = {};
  command.u.rect_c.rect = rect;
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i] = corner_color[i]; }
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.corner_radius[i] = corner_radiuses.v[i]; }
  d_add_command_to_batch(batch, command);
}

void d_add_rect_command(Rect rect, V4F32 color)
{
  V4F32 colors[UV__COUNT] = { color, color, color, color };
  d_add_rect_command_ex(rect, colors, V4F32{});
}

void d_add_texture_command(ID3D11Texture2D* texture, Rect dest_rect, Rect src_rect, B32 is_text, V4F32 text_color)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_batch* batch = draw_state->command_batch_list.last;
  if (batch == 0 || batch->command_type != D_Command_type__Texture || batch->texture != texture)
  {
    batch = d_add_new_batch(D_Command_type__Texture, texture);
  }

  D_Command command = {};
  command.u.texture_c.dest_rect  = dest_rect;
  command.u.texture_c.src_rect   = src_rect;
  command.u.texture_c.is_text    = is_text;
  command.u.texture_c.text_color = text_color;

  d_add_command_to_batch(batch, command);
}

#endif