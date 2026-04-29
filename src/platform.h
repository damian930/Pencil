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
enum OS_Event_modifier : U32 {
  OS_Event_modifier__NONE,
  OS_Event_modifier__Shift,
  OS_Event_modifier__Control,
};
typedef U32 OS_Event_modifiers;

enum OS_Event_kind {
  OS_Event_kind__NONE,
  OS_Event_kind__Mouse_went_up,
  OS_Event_kind__Mouse_went_down,
};

struct OS_Event {
  OS_Event_kind kind;
  
  V2S32 mouse_pos;
  
  OS_Event_modifiers mods;
  
  OS_Event* next;
  OS_Event* prev;
};

struct OS_Event_list {
  OS_Event* first;
  OS_Event* last;
  U64 count;
};

enum OS_Mouse_button {
  OS_Mouse_button__NONE,
  OS_Mouse_button__Left,
  OS_Mouse_button__Right,
  OS_Mouse_button__Middle, 
  OS_Mouse_button__Side_far,
  OS_Mouse_button__Side_near,
  OS_Mouse_button__Count,
};

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

#endif