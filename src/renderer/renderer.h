#ifndef RENDERER_D3D11_H
#define RENDERER_D3D11_H

#include "core/core_include.h"

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

// For uniforms
struct D3D_Rect_program_data {
  F32 window_width;
  F32 window_height;

  F32 rect_origin_x; 
  F32 rect_origin_y; 

  F32 rect_width;
  F32 rect_height;

  F32 u_border_thickness;

  F32 _padding[1]; // Padding to have the float4 (V4F32) be on the 16 byte boundary (sizeof(F32) * 4)
  
  V4F32 rect_color; 
  V4F32 border_color; 
};

// For uniforms
struct D3D_Circle_program_data {
  V4F32 color;

  F32 origin_x; 
  F32 origin_y; 
  
  F32 radius;

  F32 window_width;
  F32 window_height;

  F32 _padding[3];
};

// For uniforms
struct D3D_Texture_program_data {
  F32 vp_width;
  F32 vp_height;
 
  V2F32 dest_rect_origin;
  V2F32 dest_rect_size;

  V2F32 src_rect_origin;
  V2F32 src_rect_size;

  V2F32 src_texture_dims;
};

struct D3D_Program {
  // ID3D11InputLayout* input_layout_for_rect_program;
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11Buffer* uniform_data;
};

struct D3D_State {
  // These we get at initialisation
  IDXGIFactory2* dxgi_factory;
  IDXGIAdapter* dxgi_adapter;
  ID3D11Device* device;
  ID3D11DeviceContext* context;
  IDXGISwapChain1* swap_chain;
  
  // Draw programs
  D3D_Program draw_rect_program;
  D3D_Program draw_circle_program;
  D3D_Program draw_texture_program;

  // Per render pass data
  F32 render_viewport_width;
  F32 render_viewport_height;
};

extern global D3D_State __d3d_g_state;

// - State
D3D_State* r_get_state();
void r_init();
void r_relesase();

// - Render pass
void r_render_begin(F32 viewport_width, F32 viewport_height);
void r_render_end();

// - Drawing
void r_clear_frame_buffer(V4F32 color);
void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color);
void r_draw_rect(ID3D11RenderTargetView* rtv, Rect rect, V4F32 color);
void r_draw_rect_pro(ID3D11RenderTargetView* rtv, Rect rect, V4F32 rect_color, F32 border_line_thickness, V4F32 border_color);
void r_draw_circle(ID3D11RenderTargetView* rtv, F32 center_x, F32 center_y, F32 radius, V4F32 color);
void r_draw_texture(ID3D11RenderTargetView* target_rtv, ID3D11RenderTargetView* source_rtv, V2F32 origin);

// - Other
ID3D11RenderTargetView* r_get_frame_buffer_rtv();
ID3D11RenderTargetView* r_make_texture(U32 width, U32 height);
D3D_Texture_result r_texture_from_rtv(ID3D11RenderTargetView* rtv);
V2F32 r_get_texture_dims(ID3D11RenderTargetView* rtv);
D3D_Program r_program_from_file(
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
);

// - Misc
Image r_export_texture(Arena* arena, ID3D11RenderTargetView* rtv);
ID3D11RenderTargetView* r_load_texture_from_file(Str8 file_name);
ID3D11RenderTargetView* r_load_texture_from_image(Image image);




#endif