#ifndef OS_WIN32_H
#define OS_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"

#include "core/core_include.h"

struct OS_State;
extern OS_State __os_g_state;

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
V2S32 os_get_client_area_dims();
V2F32 os_get_mouse_pos();         // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_prev_mouse_pos();    // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_mouse_delta();
B32 os_window_should_close();

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

struct Grafics_rect_program_uniform_data {
  F32 rect_origin_x;
  F32 rect_origin_y;

  F32 rect_width;
  F32 rect_height;

  F32 window_width;
  F32 window_height;

  U32 red;
  U32 green;
  U32 blue;
  U32 alpha;

  F32 _padding[2];
};

struct D3D_program {
  // ID3D11InputLayout* input_layout_for_rect_program;
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11Buffer* uniform_data;
};

struct D3D_state {
  IDXGIFactory2* dxgi_factory;
  IDXGIAdapter* dxgi_adapter;

  ID3D11Device* device;
  ID3D11DeviceContext* context;
  
  IDXGISwapChain1* swap_chain;
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

D3D_program grafics_create_program_from_file(
  ID3D11Device* d3d_device,
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
);

ID3D11RenderTargetView* d3d_get_frame_buffer_rtv(D3D_state* d3d);
ID3D11RenderTargetView* d3d_make_rtv(D3D_state* d3d, U32 width, U32 height);

D3D_Texture_result d3d_texture_from_rtv(ID3D11RenderTargetView* rtv);
Image_buffer d3d_export_texture(Arena* arena, D3D_state* d3d, ID3D11Texture2D* src_texture);

#endif