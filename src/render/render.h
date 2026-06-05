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
#include "os/win32.h"

// Pre-defines for draw layer 
struct D_Command_batch_list; 

// todo: I dont really like it here, but i also dont have a clear place where to put this
struct Image {
  U8* data;
  U64 width_in_px;
  U64 height_in_px;
  U64 bytes_per_pixel;
  // U64 row_stride; // There are no images right now that might have extra padding
};

// - Stuff for the rect program
struct R_Rect_instance_data {
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
//
struct R_Rect_unifrom_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};  

// - Stuff for the texture program
struct R_Texture_instance_data {
  V4F32 tint;

  V2F32 dest_rect_origin;
  V2F32 dest_rect_size;
  
  V2F32 src_rect_origin;
  V2F32 src_rect_size;
  
  V2F32 src_texture_dims;

  F32 _padding_[3];
};
//
struct R_Texture_uniform_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};

// - Stuff for the line program
struct R_Line_instance_data {
  V4F32 color;
};

struct R_Line_uniform_data {
  F32 viewport_width;
  F32 viewport_height;

  F32 _padding_[2];
};

struct R_Program {
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11InputLayout* input_layout;
};

enum R_Blend_kind {
  R_Blend_kind__alpha,
  R_Blend_kind__no_blend,
  R_Blend_kind__dest_out,
  R_Blend_kind__COUNT,
};

enum R_Fill_mode : U32 {
  R_Fill_mode__solid,
  R_Fill_mode__wireframe,
  R_Fill_mode__COUNT,
};

struct R_Target {
  // This is shared for rtvs and swap chains 
  // (There are no textures right now which are not also rtvs)
  ID3D11Texture2D*        texture;
  ID3D11RenderTargetView* texture_rtv;

  // This is only present for swap chains
  HWND __win32_window_handle_for_assert;
  IDXGISwapChain1* swap_chain;
  //
  // This is optional for swap chain and only present if the window for which the swap chain is created 
  // was made transparent at its creation. (Transparent windows use different frame buffers)
  IDCompositionDevice* comp_device; 
};

struct D3D_State {
  // These we get at initialisation
  ID3D11Device*        device;
  ID3D11DeviceContext* context;
  // 
  ID3D11RasterizerState*  rasterizer_states[R_Fill_mode__COUNT];
  ID3D11BlendState*       blend_states[R_Blend_kind__COUNT];
  ID3D11SamplerState*     sampler;
  //
  // ID3D11Texture2D* magenta_black_d3d_texture;
  //
  ID3D11Buffer* rect_program_ia_buffer;
  ID3D11Buffer* rect_program_uniform_buffer;
  R_Program     rect_program;
  //
  ID3D11Buffer* texture_program_ia_buffer;
  ID3D11Buffer* texture_program_uniform_buffer;
  R_Program     texture_program;
  // 
  ID3D11Buffer* line_program_ia_buffer;
  ID3D11Buffer* line_program_uniform_buffer;
  R_Program     line_program;
};

extern global D3D_State __d3d_g_state;

// - State
D3D_State* r_get_state();
void r_init();
void r_relesase();

// - Rendering work flow (this is in the order of how it could be used)
R_Target r_attach_window(OS_Window window);
void r_prepare_canvas(R_Target* chain);
void r_submit(R_Target target, D_Command_batch_list* command_batch_list);
void r_present(R_Target target, B32 vsync);

// - Texture stuff
R_Target r_make_texture(U32 width, U32 height);
void r_release_texture(R_Target* texture);

// - Boring stuff with handles
R_Target r_target_zero_handle();
B32 r_target_match(R_Target target, R_Target other);

// - Misc
R_Program r_program_from_file(const WCHAR* shader_program_file, 
                              const char* v_shader_main_f_name, 
                              const char* p_shader_main_f_name, 
                              const D3D11_INPUT_ELEMENT_DESC* opt_desc_arr,
                              U32 desc_arr_count);
void r_clear_target(R_Target target, V4F32 color);
Image r_image_from_texture(Arena* arena, R_Target texture);
void r_export_texture(R_Target texture, Str8 file_path);
void r_export_image(Image image, Str8 file_name);
R_Target r_load_texture_from_file(Str8 file_name);
R_Target r_load_texture_from_image(Image image);
void r_copy_into_texture_from_texture(R_Target dest_texture, R_Target src_texture, B32* out_opt_is_succ);
V2F32 r_get_target_dims(R_Target target);

///////////////////////////////////////////////////////////
// - Private stuff that is not for that caller to use or care about
//
// - Extra handle checks
B32 __r_is_target_valid_target(R_Target target);
B32 __r_is_target_valid_target_chain(R_Target target);

// - Per vertex data describtions
const global 
D3D11_INPUT_ELEMENT_DESC __r_g_rect_program_input_assembler_element_desc[] = 
{
  { "RECT_00_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_00),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_10),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_01),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_11),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_X",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, origin_x),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_Y",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, origin_y),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_WIDTH",            0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, width),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_HEIGHT",           0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, height),           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_00_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_00), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_10), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_01), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_11), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORDER_COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, border_color),     D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORNER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, border_thickness), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SOFTNESS",              0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, softness),         D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};
D3D11_INPUT_ELEMENT_DESC __r_g_texture_program_input_assembler_element_desc[] = 
{
  { "TINT",             0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Texture_instance_data, tint),             D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_ORIGIN", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, dest_rect_origin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_SIZE",   0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, dest_rect_size),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_ORIGIN",  0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_rect_origin),  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_SIZE",    0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_rect_size),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_TEXTURE_DIMS", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_texture_dims), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};
D3D11_INPUT_ELEMENT_DESC __r_g_line_program_input_assembler_element_desc[] = 
{
  { "LINE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Line_instance_data, color), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

#endif