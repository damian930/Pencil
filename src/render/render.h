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

// Pre-defines for draw layer 
struct D_Command_batch_list; 

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

  V4F32 border_color;
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

enum D3D_Blend_kind {
  D3D_Blend_kind__alpha,
  D3D_Blend_kind__no_blend,
};

struct D3D_State {
  // These we get at initialisation
  ID3D11Device*        device;
  ID3D11DeviceContext* context;
  // 
  ID3D11RasterizerState*  rasterizer_state;
  ID3D11BlendState*       alpha_blend_state;
  ID3D11SamplerState*     sampler;
  //
  ID3D11Buffer* rect_program_ia_buffer;
  ID3D11Buffer* rect_program_uniform_buffer;
  D3D_Program   rect_program;
  //
  ID3D11Buffer* texture_program_ia_buffer;
  ID3D11Buffer* texture_program_uniform_buffer;
  D3D_Program   texture_program;
};

struct R_Target {
  ID3D11Texture2D*        texture;
  ID3D11RenderTargetView* texture_rtv;
};

struct R_Chain {
  HWND __win32_window_handle_for_assert;

  IDXGISwapChain1*        swap_chain;
  ID3D11Texture2D*        texture;
  ID3D11RenderTargetView* texture_rtv;
};

V2F32 r_get_swap_chain_dims(R_Chain chain);
V2F32 r_get_target_dims(R_Target target);
R_Target r_target_from_swap_chain(R_Chain chain);

extern global D3D_State __d3d_g_state;

// - State
D3D_State* r_get_state();
void r_init();
void r_relesase();

// todo: Name this section here
// R_Handle r_handle_zero();
// B32 r_handle_match(R_Handle handle, R_Handle handle);

// - Render pass
// todo: This has more to do with the swap chain, so name it better
// void r_render_begin(V2F32 vp_dims);
// void r_render_end();

// void r_submit(D_Command_batch_list* command_batch_list);

// - Clearing
void r_clear_chain(R_Chain chain, V4F32 color);
void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color);

// - Other
ID3D11RenderTargetView* r_get_frame_buffer_rtv();
ID3D11Texture2D* r_make_texture(U32 width, U32 height);
ID3D11RenderTargetView* r_rtv_from_texture(ID3D11Texture2D* texture);
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
void r_copy_into_texture_from_texture(ID3D11RenderTargetView* dest_rtv, ID3D11RenderTargetView* src_rtv);

// - Per vertex data describtions
const global 
D3D11_INPUT_ELEMENT_DESC __r_g_rect_program_input_assembler_element_desc[] = 
{
  { "RECT_00_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, color_00),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, color_10),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, color_01),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, color_11),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_X",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, origin_x),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_Y",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, origin_y),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_WIDTH",            0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, width),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_HEIGHT",           0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, height),           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_00_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, corner_radius_00), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, corner_radius_10), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, corner_radius_01), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, corner_radius_11), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORDER_COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, border_color),     D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORNER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, border_thickness), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SOFTNESS",              0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, softness),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

D3D11_INPUT_ELEMENT_DESC __r_g_texture_program_input_assembler_element_desc[] = 
{
  { "TEXT_COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, text_color),       D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_ORIGIN", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(D3D_Texture_instance_data, dest_rect_origin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_SIZE",   0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(D3D_Texture_instance_data, dest_rect_size),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_ORIGIN",  0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(D3D_Texture_instance_data, src_rect_origin),  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_SIZE",    0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(D3D_Texture_instance_data, src_rect_size),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_TEXTURE_DIMS", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(D3D_Texture_instance_data, src_texture_dims), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "IS_TEXT_TEXTURE",  0, DXGI_FORMAT_R32_UINT,           0, TypeFieldOffset(D3D_Texture_instance_data, is_text_texture),  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

#endif