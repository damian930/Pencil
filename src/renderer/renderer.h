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
 
  V2F32 origin; // I dont like this name that much, origin means the top left, in this case of a texture
  // V2F32 size; 

  // F32 _padding[2];
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

  // These are maintained, so there is no need to pass these all the time
  ID3D11RenderTargetView* current_render_target;

  // Draw programs
  D3D_Program draw_rect_program;
  D3D_Program draw_circle_program;
  D3D_Program draw_texture_program;
};

extern global D3D_State __d3d_g_state;

// - State
D3D_State* r_get_state();

// - Render pass
void d3d_render_begin(D3D_State* d3d, F32 viewport_width, F32 viewport_height);
void d3d_render_end();

// - Drawing
void d3d_clear_frame_buffer(D3D_State* d3d, V4F32 color);
void d3d_clear_rtv(D3D_State* d3d, ID3D11RenderTargetView* rtv, V4F32 color);
void d3d_draw_rect(D3D_State* d3d, ID3D11RenderTargetView* rtv, Rect rect, V4F32 color);
void d3d_draw_rect_pro(D3D_State* d3d, ID3D11RenderTargetView* rtv, Rect rect, V4F32 rect_color, F32 border_line_thickness, V4F32 border_color);
void d3d_draw_circle(D3D_State* d3d, ID3D11RenderTargetView* rtv, F32 center_x, F32 center_y, F32 radius, V4F32 color);
void d3d_draw_texture(D3D_State* d3d, ID3D11RenderTargetView* target_rtv, ID3D11RenderTargetView* source_rtv, V2F32 origin);

// - Other
ID3D11RenderTargetView* d3d_get_frame_buffer_rtv(D3D_State* d3d);
ID3D11RenderTargetView* d3d_make_rtv(D3D_State* d3d, U32 width, U32 height);
D3D_Texture_result d3d_texture_from_rtv(ID3D11RenderTargetView* rtv);
D3D_Program d3d_program_from_file(
  ID3D11Device* d3d_device,
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
);

// - Misc
Image d3d_export_texture(Arena* arena, D3D_State* d3d, ID3D11Texture2D* src_texture);




#endif