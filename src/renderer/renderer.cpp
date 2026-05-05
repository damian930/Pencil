#ifndef RENDERER_D3D11_CPP
#define RENDERER_D3D11_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "renderer.h"

// D3D
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")      // This is for the ids for the all the interfaces
#pragma comment (lib, "d3dcompiler.lib")

// DWM
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "dcomp.lib")

global D3D_State __d3d_g_state = {};

///////////////////////////////////////////////////////////
// - State
//
D3D_State* r_get_state()
{
  return &__d3d_g_state;
}

void r_init()
{
  D3D_State* d3d = r_get_state();

  HRESULT hr = S_OK;
  #define HR(hr_value) HandleLater(hr == S_OK) // todo: Remove this 

  // Factory
  {
    hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_IDXGIFactory2, (void**)&d3d->dxgi_factory);
    HR(hr);
  }
  IDXGIFactory2* dxgi_factory = d3d->dxgi_factory;

  // Adapter
  {
    hr = dxgi_factory->EnumAdapters(0, &d3d->dxgi_adapter);
    HR(hr);
  }
  IDXGIAdapter* dxgi_adapter = d3d->dxgi_adapter;

  // Device, Context
  {
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };
    hr = D3D11CreateDevice(
      dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN /*D3D_DRIVER_TYPE_HARDWARE*/, Null, 
      D3D11_CREATE_DEVICE_DEBUG, levels, ArrayCount(levels),
      D3D11_SDK_VERSION, &d3d->device, Null, &d3d->context
    );
    HR(hr);
  }
  ID3D11Device* d3d_device = d3d->device;
  ID3D11DeviceContext* d3d_context = d3d->context;   

  // todo on release: Only use this for the debug version
  // Debug
  {
    // Debug for device
    ID3D11InfoQueue* debug_q = 0;
    hr = d3d->device->QueryInterface(IID_ID3D11InfoQueue, (void**)&debug_q);
    HR(hr);

    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    // debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
    debug_q->Release();

    // Debug for dxgi
    IDXGIInfoQueue* dxgi_debug = 0;
    hr = DXGIGetDebugInterface1(Null, IID_IDXGIInfoQueue, (void**)&dxgi_debug);
    HR(hr);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    // dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
    dxgi_debug->Release();
  }

  // Swap chain
  {
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width       = 69;
    desc.Height      = 69;
    desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo      = FALSE;
    desc.SampleDesc  = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling     = DXGI_SCALING_STRETCH;//DXGI_SCALING_NONE; // todo: Learn what these do
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode   = DXGI_ALPHA_MODE_PREMULTIPLIED;//DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags       = 0;
    hr = dxgi_factory->CreateSwapChainForComposition(d3d_device, &desc, Null, &d3d->swap_chain);
    // hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, win32_state->window_handle, &desc, Null, Null, &d3d->swap_chain);
    HR(hr);
  }
  IDXGISwapChain1* d3d_swap_chain = d3d->swap_chain;

  // Rect program
  {
    B32 rect_program_suc = true;
    d3d->draw_rect_program = r_program_from_file(L"../data/shaders/draw_rect_program_shader.hlsl", "vs_main", "ps_main", &rect_program_suc);
    Handle(rect_program_suc);
  
    B32 circle_program_succ = true;
    d3d->draw_circle_program = r_program_from_file(L"../data/shaders/draw_circle_program_shader.hlsl", "vs_main", "ps_main", &circle_program_succ);
    Handle(circle_program_succ);

    B32 texture_program_succ = true;
    d3d->draw_texture_program = r_program_from_file(L"../data/shaders/draw_texture_program_shader.hlsl", "vs_main", "ps_main", &texture_program_succ);
    Handle(texture_program_succ);
  }
}

void r_relesase()
{
  // todo:
}

///////////////////////////////////////////////////////////
// - Render pass
//
void r_render_begin(F32 viewport_width, F32 viewport_height)
{
  Handle(viewport_width != 0.0f && viewport_height != 0.0f);

  D3D_State* d3d = r_get_state();

  d3d->context->ClearState(); 

  // todo: I dont like conversion from F32 to UINT here just implicitrly like this 
  // Resizing the swap chain (the frame buffer)
  HRESULT hr = d3d->swap_chain->ResizeBuffers(0, (UINT)viewport_width, (UINT)viewport_height, DXGI_FORMAT_UNKNOWN, 0);
  Handle(hr == S_OK);

  // Resizing the viewport for valid on screen px mapping
  d3d->render_viewport_width  = viewport_width;
  d3d->render_viewport_height = viewport_height;

  {
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = viewport_width;
    vp.Height   = viewport_height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    d3d->context->RSSetViewports(1, &vp);
  }

  {
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
    rasterizer_state->Release();
  }
}

void r_render_end() {}

///////////////////////////////////////////////////////////
// - Drawing
//
void r_clear_frame_buffer(V4F32 color)
{
  D3D_State* d3d = r_get_state();
  ID3D11RenderTargetView* rtv = r_get_frame_buffer_rtv();
  d3d->context->ClearRenderTargetView(rtv, color.v);
  rtv->Release();
}

void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color)
{
  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(rtv, color.v);
}

void r_draw_rect(ID3D11RenderTargetView* rtv, Rect rect, V4F32 color)
{
  r_draw_rect_pro(rtv, rect, color, 0.0f, V4F32{});
}

void r_draw_rect_pro(ID3D11RenderTargetView* rtv, Rect rect, V4F32 rect_color, F32 border_line_thickness, V4F32 border_color)
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
  D3D_State* d3d = r_get_state();
  HRESULT hr = S_OK;

  // Input assempler stage
  d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  
  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Rect_program_data u_data = {};
    u_data.window_width       = d3d->render_viewport_width;//(F32)os_get_client_area_dims().x;
    u_data.window_height      = d3d->render_viewport_height;//(F32)os_get_client_area_dims().y;
    u_data.rect_origin_x      = rect.x;
    u_data.rect_origin_y      = rect.y;
    u_data.rect_width         = rect.width;
    u_data.rect_height        = rect.height;
    u_data.u_border_thickness = border_line_thickness;
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
  d3d->context->VSSetShader(d3d->draw_rect_program.v_shader, Null, Null);
  d3d->context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  // Making sure that the viewport is set at this point. It all zeroes if not set in d3d 11.
  { 
    UINT n = 1;
    D3D11_VIEWPORT vp = {}; 
    d3d->context->RSGetViewports(&n, &vp);
    InvariantCheck(!IsMemZero(vp));
  }

  d3d->context->PSSetShader(d3d->draw_rect_program.p_shader, Null, Null);
  d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);

  d3d->context->OMSetRenderTargets(1, &rtv, Null);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
}

void r_draw_circle(ID3D11RenderTargetView* rtv, F32 center_x, F32 center_y, F32 radius, V4F32 color)
{
  D3D_State* d3d = r_get_state();
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
    u_data.window_width  = d3d->render_viewport_width;
    u_data.window_height = d3d->render_viewport_height;
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
  d3d->context->VSSetShader(d3d->draw_circle_program.v_shader, Null, Null);
  d3d->context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  // Making sure that the viewport is set at this point. It all zeroes if not set in d3d 11.
  { 
    UINT n = 1;
    D3D11_VIEWPORT vp = {}; 
    d3d->context->RSGetViewports(&n, &vp);
    InvariantCheck(!IsMemZero(vp));
  }

  d3d->context->PSSetShader(d3d->draw_circle_program.p_shader, Null, Null);
  d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);

  d3d->context->OMSetRenderTargets(1, &rtv, Null);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
}

void r_draw_texture(ID3D11RenderTargetView* target_rtv, ID3D11RenderTargetView* source_rtv, V2F32 origin)
{
  D3D_State* d3d = r_get_state();
  HRESULT hr = S_OK;

  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Texture_program_data u_data = {};
    u_data.vp_width  = d3d->render_viewport_width;
    u_data.vp_height = d3d->render_viewport_height;
    u_data.origin    = origin; 
    // u_data.size   = size;

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

  d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  d3d->context->VSSetShader(d3d->draw_texture_program.v_shader, 0, 0);
  d3d->context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  // Making sure that the viewport is set at this point. It all zeroes if not set in d3d 11.
  { 
    UINT n = 1;
    D3D11_VIEWPORT vp = {}; 
    d3d->context->RSGetViewports(&n, &vp);
    InvariantCheck(!IsMemZero(vp));
  }

  // todo: See if this sampler need to be removed later
  ID3D11SamplerState* sampler;
  {
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter        = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU      = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV      = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW      = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias    = 0;
    desc.MaxAnisotropy = 1;
    desc.MinLOD        = 0;
    desc.MaxLOD        = D3D11_FLOAT32_MAX;
    d3d->device->CreateSamplerState(&desc, &sampler);
  }
  d3d->context->PSSetSamplers(0, 1, &sampler);
  d3d->context->PSSetShader(d3d->draw_texture_program.p_shader, 0, 0);
  {
    ID3D11Resource* resource = 0;
    source_rtv->GetResource(&resource);

    ID3D11ShaderResourceView* view = 0;
    d3d->device->CreateShaderResourceView(resource, NULL, &view);
    d3d->context->PSSetShaderResources(0, 1, &view);
    d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);
    resource->Release();
    view->Release();
  }

  d3d->context->OMSetRenderTargets(1, &target_rtv, 0);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
  sampler->Release();
}

///////////////////////////////////////////////////////////
// - Other
//
ID3D11RenderTargetView* r_get_frame_buffer_rtv()
{
  D3D_State* d3d = r_get_state();
  ID3D11RenderTargetView* frame_buffer_rtv = 0;
  ID3D11Texture2D* backbuffer;
  d3d->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
  d3d->device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &frame_buffer_rtv);
  backbuffer->Release();
  return frame_buffer_rtv;
}

ID3D11RenderTargetView* r_make_rtv(U32 width, U32 height)
{
  D3D_State* d3d = r_get_state();

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
    Handle(hr == S_OK);
  }

  ID3D11RenderTargetView* rtv = 0;
  {
    HRESULT hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &rtv);
    Handle(hr == S_OK);
  }

  texture->Release();
  return rtv;
}

D3D_Texture_result r_texture_from_rtv(ID3D11RenderTargetView* rtv)
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

D3D_Program r_program_from_file(
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
) {
  D3D_State* d3d = r_get_state();

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
    hr = d3d->device->CreateVertexShader(v_blob->GetBufferPointer(), v_blob->GetBufferSize(), Null, &v_shader);
    if (hr != S_OK) { is_succ = false; }
    hr = d3d->device->CreatePixelShader(p_blob->GetBufferPointer(), p_blob->GetBufferSize(), Null, &p_shader);
    if (hr != S_OK) { is_succ = false; }
    
    // D3D11_INPUT_ELEMENT_DESC layout_decs = {};
    // hr = d3d_device->CreateInputLayout(&layout_decs, 0, v_blob->GetBufferPointer(), v_blob->GetBufferSize(), &grafics.input_layout_for_rect_program);
    // HR(hr);
  }

  if (p_blob != 0) { p_blob->Release(); }  
  if (v_blob != 0) { v_blob->Release(); }  

  // // Creating a uniform buffer
  // D3D11_BUFFER_DESC uniform_desc = {};
  // uniform_desc.ByteWidth      = sizeof(D3D_Rect_program_data);
  // uniform_desc.Usage          = D3D11_USAGE_DYNAMIC;
  // uniform_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  // uniform_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  // d3d_device->CreateBuffer(&uniform_desc, NULL, &grafics.uniform_buffer_for_rect_program);

  if (out_opt_is_succ != 0) { *out_opt_is_succ = is_succ; }
  D3D_Program program = {};
  program.v_shader     = v_shader;
  program.p_shader     = p_shader;
  return program;
}

///////////////////////////////////////////////////////////
// - Misc
//
Image r_export_texture(Arena* arena, ID3D11Texture2D* src_texture)
{
  D3D_State* d3d = r_get_state();
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

  Image image = {};
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



#endif