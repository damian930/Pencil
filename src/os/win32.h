#ifndef OS_WIN32_H
#define OS_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"

#include "core/core_include.h"

struct OS_State;
extern OS_State __os_g_state;

///////////////////////////////////////////////////////////
// - Windowing
//
struct OS_Window {
  WNDCLASSEXA window_class;
  HWND handle;

  B32 shoud_close;
};

///////////////////////////////////////////////////////////
// - Events
// 
enum Key : U32 {
  Key__NONE,

  // Mods
  Key__Shift,
  Key__Control,

  // Letters 
  Key__A,              // 'a'   shifted: 'A'
  Key__B,              // 'b'   shifted: 'B'
  Key__C,              // 'c'   shifted: 'C'
  Key__D,              // 'd'   shifted: 'D'
  Key__E,              // 'e'   shifted: 'E'
  Key__F,              // 'f'   shifted: 'F'
  Key__G,              // 'P'   shifted: 'P'
  Key__H,              // 'h'   shifted: 'H'
  Key__I,              // 'i'   shifted: 'I'
  Key__J,              // 'j'   shifted: 'J'
  Key__K,              // 'k'   shifted: 'K'
  Key__L,              // 'l'   shifted: 'L'
  Key__M,              // 'm'   shifted: 'M'
  Key__N,              // 'n'   shifted: 'N'
  Key__O,              // 'o'   shifted: 'O'
  Key__P,              // 'p'   shifted: 'P'
  Key__Q,              // 'q'   shifted: 'Q'
  Key__R,              // 'r'   shifted: 'R'
  Key__S,              // 's'   shifted: 'S'
  Key__T,              // 't'   shifted: 'T'
  Key__U,              // 'u'   shifted: 'U'
  Key__V,              // 'v'   shifted: 'V'
  Key__W,              // 'w'   shifted: 'W'
  Key__X,              // 'x'   shifted: 'X'
  Key__Y,              // 'y'   shifted: 'Y'
  Key__Z,              // 'z'   shifted: 'Z'

  // Numbers
  Key__0,              // '0'   shifted: ')'
  Key__1,              // '1'   shifted: '!'
  Key__2,              // '2'   shifted: '@'
  Key__3,              // '3'   shifted: '#'
  Key__4,              // '4'   shifted: '$'
  Key__5,              // '5'   shifted: '%'
  Key__6,              // '6'   shifted: '^'
  Key__7,              // '7'   shifted: '&'
  Key__8,              // '8'   shifted: '*'
  Key__9,              // '9'   shifted: '('

  // Other printable 
  Key__Space,          // ' '   shifted: ' '
  Key__Backtick,       // '`'   shifted: '~'
  Key__Minus,          // '-'   shifted: '_'
  Key__Equals,         // '='   shifted: '+'
  Key__Left_bracket,   // '['   shifted: '{'
  Key__Right_bracket,  // ']'   shifted: '}'
  Key__Backslash,      // '\'   shifted: '|'
  Key__Semicolon,      // ';'   shifted: ':'
  Key__Apostrophe,     // '\''  shifted: '"'
  Key__Comma,          // ','   shifted: '<'
  Key__Period,         // '.'   shifted: '>'
  Key__Slash,          // '/'   shifted: '?'

  // Other 
  Key__Left_arrow,
  Key__Right_arrow,
  Key__Up_arrow,
  Key__Down_arrow,
  Key__Home,
  Key__End,
  Key__Page_up,
  Key__Page_down,
  Key__Backspace,
  Key__Delete,
  Key__Insert,
  Key__Escape,
  Key__Tab,
  Key__Enter,
  Key__Caps_lock,

  Key__COUNT,
};

enum Mouse_button : U32 {
  Mouse_button__NONE,
  Mouse_button__Left,
  Mouse_button__Right,
  Mouse_button__Middle,
  Mouse_button__Side_far,
  Mouse_button__Side_near,
  Mouse_button__COUNT,
};

enum OS_Event_modifier : U32 {
  OS_Event_modifier__NONE,
  OS_Event_modifier__Shift,
  OS_Event_modifier__Control,
};
typedef U32 OS_Event_modifiers;

enum OS_Event_kind {
  OS_Event_kind__NONE,
  OS_Event_kind__Key,
  OS_Event_kind__Mouse_went_up,
  OS_Event_kind__Mouse_went_down,
};

struct OS_Key_state {
  Key key;

  B8 is_up;
  B8 is_down;

  B8 was_up;
  B8 was_down;

  B8 is_clicked;
};

struct OS_Mouse_button_state {
  Mouse_button button;
  
  B8 is_up;
  B8 is_down;

  B8 was_up;
  B8 was_down;
};

// - State
void os_init();
void os_release();
OS_State* os_get_state();

// - Frame
void os_frame_begin();
void os_frame_end();

// - Windowing
V2F32 os_get_window_dims();
V2F32 os_get_client_area_dims();
V2F32 os_get_mouse_pos();         // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_prev_mouse_pos();    // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_mouse_delta();
B32 os_window_should_close();
void os_window_maximize();
void os_window_minimize();
// void os_window_set_top_most(B32 b);

// - Inputs for keyboard
OS_Key_state os_get_key_state(Key key);
B32 os_key_down(Key key);
B32 os_key_up(Key key);
B32 os_key_went_down(Key key);
B32 os_key_went_up(Key key);
Str8 str8_from_os_key(Key key);

// - Inputs for mouse
OS_Mouse_button_state os_get_mouse_button_state(Mouse_button button);
B32 os_mouse_button_down(Mouse_button button);
B32 os_mouse_button_up(Mouse_button button);
B32 os_mouse_button_went_down(Mouse_button button);
B32 os_mouse_button_went_up(Mouse_button button);

// - Time
Readable_time os_get_readable_time();
Time os_get_time_ms();
U64 os_get_perf_counter();
U64 os_get_perf_freq_per_sec();
void os_sleep(U64 ms);
// U64 os_get_mouse_double_click_max_time_ms();
// U64 os_get_keyboard_initial_repeat_delay();
// U64 os_get_keyboard_subsequent_repeat_delay();

///////////////////////////////////////////////////////////
// - Grafics
//
#include "d3d11.h"
#include "dxgi.h"
#include "dxgidebug.h"
#include "dxgi1_3.h"
#include "d3dcompiler.h"
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")    // This is for the ids for the all the interfaces
#pragma comment (lib, "d3dcompiler.lib")

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

struct D3D_Circle_program_data {
  V4F32 color;

  F32 origin_x; 
  F32 origin_y; 
  
  F32 radius;

  F32 window_width;
  F32 window_height;

  F32 _padding[3];
};

struct D3D_Program {
  // ID3D11InputLayout* input_layout_for_rect_program;
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11Buffer* uniform_data;
};

struct D3D_State {
  IDXGIFactory2* dxgi_factory;
  IDXGIAdapter* dxgi_adapter;

  ID3D11Device* device;
  ID3D11DeviceContext* context;
  
  IDXGISwapChain1* swap_chain;

  // Some programs here for now, dont know where to put them
  D3D_Program rect_program;
  D3D_Program circle_program;
};

// todo:
// note: I wanted to maybe make this be just a zero id for texture like in opengl, 
//       but since this is a pointer I cant just return a 0 pointer
//       or have a struct to reference as 0, since you cant make an instance of this type other than a 
//       pointer to it.
struct D3D_Texture_result {
  B32 succ;  
  ID3D11Texture2D* texture;
};

struct Image_buffer {
  U8* data;
  U64 width_in_px;
  U64 height_in_px;
  U64 bytes_per_pixel;
  // U64 row_stride; // There are no images right now that might have extra padding
};

D3D_Program d3d_program_from_file(
  ID3D11Device* d3d_device,
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
);

ID3D11RenderTargetView* d3d_get_frame_buffer_rtv(D3D_State* d3d);
ID3D11RenderTargetView* d3d_make_rtv(D3D_State* d3d, U32 width, U32 height);

D3D_Texture_result d3d_texture_from_rtv(ID3D11RenderTargetView* rtv);
Image_buffer d3d_export_texture(Arena* arena, D3D_State* d3d, ID3D11Texture2D* src_texture);

// todo: This is testing for quick drawing calls with d3d
void d3d_render_begin(D3D_State* d3d, F32 viewport_width, F32 viewport_height)
{
  d3d->context->ClearState(); 

  // Viewport fro this render pass
  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  vp.Width    = viewport_width;
  vp.Height   = viewport_height;
  vp.MinDepth = 0;
  vp.MaxDepth = 1;
  d3d->context->RSSetViewports(1, &vp);
}

void d3d_render_end(D3D_State* d3d) 
{



}

void d3d_clear_frame_buffer(D3D_State* d3d, V4F32 color)
{
  ID3D11RenderTargetView* rtv = d3d_get_frame_buffer_rtv(d3d);
  d3d->context->ClearRenderTargetView(rtv, color.v);
  rtv->Release();
}

// todo: Remove this from here
void d3d_draw_rect_pro(D3D_State* d3d, ID3D11RenderTargetView* rtv, Rect rect, V4F32 rect_color, F32 border_line_thickness, V4F32 border_color);

void d3d_draw_rect(D3D_State* d3d, ID3D11RenderTargetView* rtv, Rect rect, V4F32 color)
{
  d3d_draw_rect_pro(d3d, rtv, rect, color, 0.0f, V4F32{});
}

void d3d_draw_rect_pro(D3D_State* d3d, ID3D11RenderTargetView* rtv, Rect rect, V4F32 rect_color, F32 border_thickness, V4F32 border_color)
{ 
  // note: Default blending is used for now, so no alpha blend
  
  /* note:
    At first i wanted to reset all the state, 
    there is an issue with that since sometimes i need the state to persist,
    like with viewport. Most of the state for the rendering pipeline i dont understand.
    I dont know what it is for and dont use in these calls. 
    Not sure if clearing it would be better or not. So i guess for now i will
    just only work with the sub set of the rendering pipeline that i know and am using.
  */
  HRESULT hr = S_OK;

  // Input assempler stage
  d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  
  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Rect_program_data u_data = {};
    u_data.window_width       = (F32)os_get_client_area_dims().x;
    u_data.window_height      = (F32)os_get_client_area_dims().y;
    u_data.rect_origin_x      = rect.x;
    u_data.rect_origin_y      = rect.y;
    u_data.rect_width         = rect.width;
    u_data.rect_height        = rect.height;
    u_data.u_border_thickness = border_thickness;
    u_data.rect_color         = rect_color;
    u_data.border_color       = border_color;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(u_data);
    desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d3d->device->CreateBuffer(&desc, NULL, &uniform_buffer);
    
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = d3d->context->Map((ID3D11Resource*)uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (hr == S_OK) {
      memcpy(mapped.pData, &u_data, sizeof(u_data));
      d3d->context->Unmap((ID3D11Resource*)uniform_buffer, 0);
    } else {
      Assert(false); // note: Dont handle this better than that cause it is not for the user, it shoud just work in the final version regardless
    }
  }

  // Vertex shader stage 
  d3d->context->VSSetShader(d3d->rect_program.v_shader, Null, Null);
  d3d->context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  // todo: Test this with culling off
  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    hr = d3d->device->CreateRasterizerState(&desc, &rasterizer_state);
    Assert(hr == S_OK);
  }
  d3d->context->RSSetState(rasterizer_state);
  
  // Making sure that the viewport is set at this point. It all zeroes if not set in d3d 11.
  { 
    UINT n = 1;
    D3D11_VIEWPORT vp = {}; 
    d3d->context->RSGetViewports(&n, &vp);
    Assert(!IsMemZero(vp));
  }

  d3d->context->PSSetShader(d3d->rect_program.p_shader, Null, Null);
  d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);

  d3d->context->OMSetRenderTargets(1, &rtv, Null);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
  rasterizer_state->Release();

  // todo: I dont know if we need to reset all the thing we set in this func 
  // to the thing that were set before this func. 
}

void d3d_draw_circle(D3D_State* d3d, ID3D11RenderTargetView* rtv, F32 center_x, F32 center_y, F32 radius, V4F32 color)
{
  HRESULT hr = S_OK;

  // Input assempler stage
  d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  
  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Circle_program_data u_data = {};
    u_data.origin_x      = center_x; 
    u_data.origin_y      = center_y; 
    u_data.radius        = radius;
    u_data.window_width  = (F32)os_get_client_area_dims().x;
    u_data.window_height = (F32)os_get_client_area_dims().y;
    u_data.color         = color; 

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(u_data);
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags      = {};
    desc.StructureByteStride = {};
    d3d->device->CreateBuffer(&desc, NULL, &uniform_buffer);
    
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = d3d->context->Map((ID3D11Resource*)uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (hr == S_OK) {
      memcpy(mapped.pData, &u_data, sizeof(u_data));
      d3d->context->Unmap((ID3D11Resource*)uniform_buffer, 0);
    } else {
      Assert(false); // note: Dont handle this better than that cause it is not for the user, it shoud just work in the final version regardless
    }
  }

  // Vertex shader stage 
  d3d->context->VSSetShader(d3d->circle_program.v_shader, Null, Null);
  d3d->context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  // todo: Test this with culling off
  // todo: This seems to be reused in between the draw calls, might also be cashed
  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    hr = d3d->device->CreateRasterizerState(&desc, &rasterizer_state);
    Assert(hr == S_OK);
  }
  d3d->context->RSSetState(rasterizer_state);
  
  // Making sure that the viewport is set at this point. It all zeroes if not set in d3d 11.
  { 
    UINT n = 1;
    D3D11_VIEWPORT vp = {}; 
    d3d->context->RSGetViewports(&n, &vp);
    Assert(!IsMemZero(vp));
  }

  d3d->context->PSSetShader(d3d->circle_program.p_shader, Null, Null);
  d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);

  d3d->context->OMSetRenderTargets(1, &rtv, Null);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
  rasterizer_state->Release();
}

#endif