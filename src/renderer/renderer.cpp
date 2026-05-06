#ifndef RENDERER_D3D11_CPP
#define RENDERER_D3D11_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "renderer.h"

// todo: Remove this from this layer
#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

// D3D
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")      // This is for the ids for the all the interfaces
#pragma comment (lib, "d3dcompiler.lib")

// DWM
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "dcomp.lib")

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "__third_party/stb/stb_image.h"
#endif

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

  // Alpha blending
  {
    ID3D11BlendState* blend_state = 0;
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    d3d->device->CreateBlendState(&desc, &blend_state);
    d3d->context->OMSetBlendState(blend_state, Null, ~0U);
    blend_state->Release();
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

  // todo: I shoud do this in each call to clear the non retained state per render
  //       pass to not have weird state bugs from draw call to draw call
  d3d->context->OMSetRenderTargets(0, 0, Null);

  uniform_buffer->Release();
}

void r_draw_texture(ID3D11RenderTargetView* dest_rtv, Rect rect_in_dest, ID3D11RenderTargetView* src_rtv, Rect rect_in_src)
{
  D3D_State* d3d = r_get_state();
  HRESULT hr = S_OK;

  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Texture_program_data u_data = {};
    u_data.vp_width         = d3d->render_viewport_width;
    u_data.vp_height        = d3d->render_viewport_height;
    u_data.dest_rect_origin = v2f32(rect_in_dest.x, rect_in_dest.y);
    u_data.dest_rect_size   = v2f32(rect_in_dest.width, rect_in_dest.height);
    u_data.src_rect_origin  = v2f32(rect_in_src.x, rect_in_src.y);
    u_data.src_rect_size    = v2f32(rect_in_src.width, rect_in_src.height);
    u_data.src_texture_dims = r_get_texture_dims(src_rtv);

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
    src_rtv->GetResource(&resource);

    ID3D11ShaderResourceView* view = 0;
    d3d->device->CreateShaderResourceView(resource, NULL, &view);
    d3d->context->PSSetShaderResources(0, 1, &view);
    d3d->context->PSSetConstantBuffers(0, 1, &uniform_buffer);
    view->Release();
    resource->Release();
  }

  d3d->context->OMSetRenderTargets(1, &dest_rtv, 0);

  d3d->context->Draw(4, 0);

  uniform_buffer->Release();
  sampler->Release();
}

void r_draw_text(ID3D11RenderTargetView* dest_rtv, Str8 text, V2F32 pos, FP_Font font, V4F32 color)
{
  // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y, 100, 1), green_f());
  // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y + font.ascent + font.descent, 100, 1), green_f());

  F32 origin_y = pos.y + font.ascent;
  F32 x_offset = 0.0f;

  // r_draw_rect(dest_rtv, rect_make(pos.x, origin_y, 100, 1), green_f());

  for (U64 ch_index = 0; ch_index < text.count; ch_index += 1)
  {
    U8 ch = text.data[ch_index];
    FP_Codepoint_data glyph_data = fp_get_glyph_data(font, ch); 

    F32 origin_x = pos.x + x_offset;

    // Just puttin them 1 next to another
    Rect dest_rect = {};
    dest_rect.x      = origin_x + glyph_data.bearing_x;
    dest_rect.y      = origin_y - glyph_data.bearing_y;
    dest_rect.width  = glyph_data.rect_on_atlas.width;
    dest_rect.height = glyph_data.rect_on_atlas.height;
    
    r_draw_texture(
      dest_rtv, dest_rect,
      font.atlas_texture, glyph_data.rect_on_atlas
    );

    F32 advance = glyph_data.advance;
    if (ch_index < text.count - 1)
    {
      FP_Kerning_entry entry = fp_get_kerning(font, ch, text.data[ch_index + 1]);
      if (!IsMemZero(entry)) { advance += entry.advance; }
    } 
    x_offset += advance; 
  }
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

ID3D11RenderTargetView* r_make_texture(U32 width, U32 height)
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

// note: The texture has to be released later
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

V2F32 r_get_texture_dims(ID3D11RenderTargetView* rtv)
{
  D3D_Texture_result texture_res = r_texture_from_rtv(rtv);
  Handle(texture_res.succ);

  D3D11_TEXTURE2D_DESC desc = {};
  texture_res.texture->GetDesc(&desc);

  texture_res.texture->Release();
  V2F32 dims = v2f32((F32)desc.Width, (F32)desc.Height);
  return dims;
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
Image r_image_from_texture(Arena* arena, ID3D11RenderTargetView* rtv)
{
  D3D_State* d3d = r_get_state();
  HRESULT hr = S_OK;

  // Stuff to clear at the end
  Scratch          scratch      = get_scratch(0, 0);
  ID3D11Resource*  resource     = 0;
  ID3D11Texture2D* texture      = 0;
  ID3D11Texture2D* copy_texture = 0;

  rtv->GetResource(&resource);
  hr = resource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);
  Handle(hr == S_OK);

  U64 texture_height = 0;
  U64 texture_width  = 0;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    desc.BindFlags      = 0;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    texture_height = (U64)desc.Height;
    texture_width  = (U64)desc.Width;
    format         = desc.Format;

    hr = d3d->device->CreateTexture2D(&desc, 0, &copy_texture);
    HandleLater(hr == S_OK);

    d3d->context->CopyResource((ID3D11Resource*)copy_texture, (ID3D11Resource*)texture);
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
  texture->Release();
  resource->Release();
  end_scratch(&scratch);

  return image;
}

void r_export_texture(ID3D11RenderTargetView* rtv, Str8 file_path)
{
  Scratch scratch = get_scratch(0, 0);
  Image image = r_image_from_texture(scratch.arena, rtv);

  Str8 file_path_nt = str8_copy_alloc(scratch.arena, file_path);
  int succ = stbi_write_png((char*)file_path_nt.data, image.width_in_px, image.height_in_px, image.bytes_per_pixel, image.data, image.width_in_px * image.bytes_per_pixel);
  Handle(succ);
  end_scratch(&scratch);
}

ID3D11RenderTargetView* r_load_texture_from_file(Str8 file_name)
{
  Scratch scratch = get_scratch(0, 0);
  D3D_State* d3d = r_get_state();
  ID3D11RenderTargetView* result_rtv = 0;

  Str8 file_name_nt = str8_copy_alloc(scratch.arena, file_name);

  int width = 0;
  int height = 0;
  int n_channels = 0;
  U8* image_bytes = stbi_load((char*)file_name_nt.data, &width, &height, &n_channels, 4);
  
  if (image_bytes)
  {
    Image image = {};
    image.data            = image_bytes;
    image.width_in_px     = (U64)width;
    image.height_in_px    = (U64)height;
    image.bytes_per_pixel = (U64)n_channels;
    result_rtv = r_load_texture_from_image(image);
  }

  end_scratch(&scratch);
  return result_rtv;
}

ID3D11RenderTargetView* r_load_texture_from_image(Image image)
{
  D3D_State* d3d = r_get_state();
  ID3D11RenderTargetView* result_rtv = 0;

  if (image.bytes_per_pixel != 4) { NotImplemented(); } // Only DXGI_FORMAT_R8G8B8A8_UNORM supported for now

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width      = (UINT)image.width_in_px; // todo: I dont like the U64 to UINT conversion here
  desc.Height     = (UINT)image.height_in_px; // todo: I dont like the U64 to UINT conversion here
  desc.MipLevels  = 1;
  desc.ArraySize  = 1;
  desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc = { 1, 0 };
  desc.Usage      = D3D11_USAGE_DEFAULT;
  desc.BindFlags  = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA data = {};
  data.pSysMem     = image.data;
  data.SysMemPitch = (UINT)(image.width_in_px * image.bytes_per_pixel); // todo: I dont like the U64 to uint onversion here

  ID3D11Texture2D* texture = 0;
  HRESULT create_succ = d3d->device->CreateTexture2D(&desc, &data, &texture);
  
  if (create_succ == S_OK)
  {
    HRESULT rtv_succ = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, NULL, &result_rtv);
    Handle(rtv_succ == S_OK);
    texture->Release();
  }

  return result_rtv;
}

// note: This is a very shitty function ))
void r_copy_from_texture_to_texture(
  ID3D11RenderTargetView* dest_rtv, 
  ID3D11RenderTargetView* src_rtv
) {
  D3D_State* d3d = r_get_state();
  
  ID3D11Resource* dest_resource = 0;
  dest_rtv->GetResource(&dest_resource);

  ID3D11Resource* src_resource = 0;
  src_rtv->GetResource(&src_resource);
  
  d3d->context->CopyResource(dest_resource, src_resource);
}

void r_scissoring_begin(Rect rect)
{
  D3D_State* d3d = r_get_state();

  ID3D11RasterizerState* old_rasterizer_state = 0;
  d3d->context->RSGetState(&old_rasterizer_state);

  D3D11_RASTERIZER_DESC desc = {};
  old_rasterizer_state->GetDesc(&desc);
  desc.ScissorEnable = true;

  old_rasterizer_state->Release();

  ID3D11RasterizerState* new_rasterizer_state = 0;
  HRESULT hr1 = d3d->device->CreateRasterizerState(&desc, &new_rasterizer_state);
  Handle(hr1 == S_OK);

  d3d->context->RSSetState(new_rasterizer_state);
  new_rasterizer_state->Release();
  
  D3D11_RECT scissorRect = {};
  scissorRect.left   = (LONG)rect.x;
  scissorRect.top    = (LONG)rect.y;
  scissorRect.right  = (LONG)(rect.x + rect.width);
  scissorRect.bottom = (LONG)(rect.y + rect.height);
  d3d->context->RSSetScissorRects(1, &scissorRect);
}

void r_scissoring_end()
{
  D3D_State* d3d = r_get_state();

  ID3D11RasterizerState* old_rasterizer_state = 0;
  d3d->context->RSGetState(&old_rasterizer_state);

  D3D11_RASTERIZER_DESC desc = {};
  old_rasterizer_state->GetDesc(&desc);
  desc.ScissorEnable = false;

  old_rasterizer_state->Release();

  ID3D11RasterizerState* new_rasterizer_state = 0;
  HRESULT hr1 = d3d->device->CreateRasterizerState(&desc, &new_rasterizer_state);
  Handle(hr1 == S_OK);

  d3d->context->RSSetState(new_rasterizer_state);
  new_rasterizer_state->Release();
  
  d3d->context->RSSetScissorRects(0, 0);
}

#endif