#ifndef PLATFORM_H
#define PLATFORM_H

// todo: Deal with includes here

#include "core/core_include.h"

// todo: Not sure if i want to have these here
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"

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

// struct OS_Event {
//   OS_Event_kind kind;
  
//   // Fat stuff for now
//   OS_Key key;
//   B8 key_went_up;
//   B8 key_went_down;
//   B8 key_repeat_on_hold;
//   //
//   V2S32 mouse_pos;
//   //
//   OS_Event_modifiers mods;
  
//   OS_Event* next;
//   OS_Event* prev;
// };

// struct OS_Event_list {
//   OS_Event* first;
//   OS_Event* last;
//   U64 count;
// };

// enum OS_Mouse_button {
//   OS_Mouse_button__NONE,
//   OS_Mouse_button__Left,
//   OS_Mouse_button__Right,
//   OS_Mouse_button__Middle, 
//   OS_Mouse_button__Side_far,
//   OS_Mouse_button__Side_near,
//   OS_Mouse_button__Count,
// };

// struct OS_Mouse_button_state {
//   B32 is_down;
// };

// struct OS_Mouse_button_states {
//   OS_Mouse_button_state states[OS_Mouse_button__Count];
// };

///////////////////////////////////////////////////////////
// - Misc
//
// V2S32 os_get_client_area_dims();
// V2S32 os_get_mouse_pos();

struct OS_State;
extern OS_State __os_g_state;


// note: I have no idea how to use this or to abstract over it,
//       so i will just be using this straight up like this i guess for now
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

// note: This has to then later be released with ->Release() call 
// todo: I am not sure why i would need a call for this, since storing a frame buffer in the flip model is kind of weird
ID3D11RenderTargetView* d3d_get_frame_buffer_rtv(D3D_state* d3d)
{
  ID3D11RenderTargetView* frame_buffer_rtv = 0;
  ID3D11Texture2D* backbuffer;
  d3d->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
  d3d->device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &frame_buffer_rtv);
  backbuffer->Release();
  return frame_buffer_rtv;
}

ID3D11RenderTargetView* d3d_make_rtv(D3D_state* d3d, U32 width, U32 height)
{
  ID3D11Texture2D* texture = 0;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = d3d->device->CreateTexture2D(&desc, Null, &texture);
    Assert(hr == S_OK);
  }

  ID3D11RenderTargetView* rtv = 0;
  {
    HRESULT hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &rtv);
    Assert(hr == S_OK);
  }

  texture->Release();
  return rtv;
}

D3D_program grafics_create_program_from_file(
  ID3D11Device* d3d_device,
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
) {
  B32 is_succ = true;

  // Getting v shader compiled
  ID3DBlob* v_blob = 0;
  if (is_succ)
  {
    ID3DBlob* error_blob = 0;

    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "vs_main", "vs_5_0", Null, Null, &v_blob, &error_blob
    );

    if (error_blob != 0 || hr != S_OK)
    {
      if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
      is_succ = false;
    }

    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3DBlob* p_blob = 0;
  if (is_succ)
  {
    ID3DBlob* error_blob = 0;
  
    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "ps_main", "ps_5_0", Null, Null, &p_blob, &error_blob
    );

    if (error_blob != 0 || hr != S_OK)
    {
      if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
      is_succ = false;
    }

    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3D11VertexShader* v_shader = 0;
  ID3D11PixelShader* p_shader = 0;
  if (is_succ)
  {
    HRESULT hr = S_OK;
    hr = d3d_device->CreateVertexShader(v_blob->GetBufferPointer(), v_blob->GetBufferSize(), Null, &v_shader);
    if (hr != S_OK) { is_succ = false; }
    hr = d3d_device->CreatePixelShader(p_blob->GetBufferPointer(), p_blob->GetBufferSize(), Null, &p_shader);
    if (hr != S_OK) { is_succ = false; }
    
    // D3D11_INPUT_ELEMENT_DESC layout_decs = {};
    // hr = d3d_device->CreateInputLayout(&layout_decs, 0, v_blob->GetBufferPointer(), v_blob->GetBufferSize(), &grafics.input_layout_for_rect_program);
    // HR(hr);
  }

  if (p_blob != 0) { p_blob->Release(); }  
  if (v_blob != 0) { v_blob->Release(); }  

  // // Creating a uniform buffer
  // D3D11_BUFFER_DESC uniform_desc = {};
  // uniform_desc.ByteWidth      = sizeof(Grafics_rect_program_uniform_data);
  // uniform_desc.Usage          = D3D11_USAGE_DYNAMIC;
  // uniform_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  // uniform_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  // d3d_device->CreateBuffer(&uniform_desc, NULL, &grafics.uniform_buffer_for_rect_program);

  if (out_opt_is_succ != 0) { *out_opt_is_succ = is_succ; }
  D3D_program program = {};
  program.v_shader     = v_shader;
  program.p_shader     = p_shader;
  return program;
}

// Image gpu_image_fresh = LoadImageFromTexture(G->draw_texture_always_fresh.texture);
// Assert(gpu_image_fresh.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
struct Image_buffer {
  U8* data;
  U64 width_in_px;
  U64 height_in_px;
  U64 bytes_per_pixel;
  // U64 row_stride; // There are no images right now that might have extra padding
};

Image_buffer d3d_export_texture(Arena* arena, D3D_state* d3d, ID3D11Texture2D* src_texture)
{
  HRESULT hr = S_OK;

  Scratch scratch = get_scratch(0, 0);
  ID3D11Texture2D* copy_texture = 0;

  U64 texture_height = 0;
  U64 texture_width  = 0;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    src_texture->GetDesc(&desc);
    desc.BindFlags      = 0;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    texture_height = (U64)desc.Height;
    texture_width  = (U64)desc.Width;
    format         = desc.Format;

    hr = d3d->device->CreateTexture2D(&desc, 0, &copy_texture);
    HandleLater(hr == S_OK);

    d3d->context->CopyResource((ID3D11Resource*)copy_texture, (ID3D11Resource*)src_texture);
  }

  // note: For now only this one
  HandleLater(format == DXGI_FORMAT_R8G8B8A8_UNORM);
  U64 bytes_per_pixel = 4;

  Image_buffer image = {};
  {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = d3d->context->Map((ID3D11Resource*)copy_texture, 0, D3D11_MAP_READ, 0, &mapped);
    {
      U64 size_for_image = texture_height * texture_width * bytes_per_pixel;
      image.bytes_per_pixel = bytes_per_pixel;
      // image.row_stride      = (U64)mapped.RowPitch; 
      image.width_in_px     = texture_width;
      image.height_in_px    = texture_height;
      image.data            = ArenaPushArr(arena, U8, size_for_image);

      HandleLater(hr == S_OK);
      if (mapped.pData)
      {
        for (U64 row_index = 0; row_index < texture_height; row_index += 1)
        {
          memcpy(
            image.data + row_index * texture_width * bytes_per_pixel, 
            (U8*)mapped.pData + row_index * mapped.RowPitch,
            texture_width * bytes_per_pixel
          );
        }
      }
    }
    d3d->context->Unmap((ID3D11Resource*)copy_texture, 0);
  }

  copy_texture->Release();
  end_scratch(&scratch);

  return image;
}

// note: I wanted to maybe make this be just a zero id for texture like in opengl, 
//       but since this is a pointer I cant just return a 0 pointer
//       or have a struct to reference as 0, since you cant make an instance of this type other than a 
//       pointer to it.
struct D3D_Texture_result {
  B32 succ;  
  ID3D11Texture2D* texture;
};

D3D_Texture_result d3d_texture_from_rtv(ID3D11RenderTargetView* rtv)
{
  HRESULT hr = S_OK;
  
  ID3D11Resource* resource = 0;
  rtv->GetResource(&resource);

  ID3D11Texture2D* texture = 0;
  hr = resource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);

  D3D_Texture_result result = {};
  result.texture = texture;
  result.succ    = (hr == S_OK);

  resource->Release();
  return result;
}


#endif