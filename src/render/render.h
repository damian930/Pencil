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

  F32 border_thickness;
  F32 softness;

  F32 _padding_[2];
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

#endif