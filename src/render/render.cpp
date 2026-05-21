#ifndef RENDERER_D3D11_CPP
#define RENDERER_D3D11_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "render.h"

#include "draw/draw.h"

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

#define HR(cond) Handle(cond == S_OK)

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

  // Device, Context
  {
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };
    UINT flags = 0;
    #if DEBUG_MODE
    flags = D3D11_CREATE_DEVICE_DEBUG; 
    #endif

    hr = D3D11CreateDevice(
      Null, D3D_DRIVER_TYPE_HARDWARE, Null,  
      flags, levels, ArrayCount(levels),
      D3D11_SDK_VERSION, &d3d->device, Null, &d3d->context
    );
    HR(hr);
  }

  // Debug
  #if DEBUG_MODE
  {
    // Debug for device
    ID3D11InfoQueue* debug_q = 0;
    hr = d3d->device->QueryInterface(IID_ID3D11InfoQueue, (void**)&debug_q);
    HR(hr);

    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    debug_q->Release();

    // Debug for dxgi
    IDXGIInfoQueue* dxgi_debug = 0;
    hr = DXGIGetDebugInterface1(Null, IID_IDXGIInfoQueue, (void**)&dxgi_debug);
    HR(hr);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    dxgi_debug->Release();
  }
  #endif

  // Rasterizer state
  {
    // No culling, this makes all the triangles appear, not only the once that follow a specific clock direction (meaning clock-wise or counter clock-wise)
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode        = D3D11_FILL_SOLID;
    desc.CullMode        = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    hr = d3d->device->CreateRasterizerState(&desc, &d3d->rasterizer_state);
    HR(hr);
  }

  // Alpha blending
  {
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    d3d->device->CreateBlendState(&desc, &d3d->alpha_blend_state);
  }

  // Sampler
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
    d3d->device->CreateSamplerState(&desc, &d3d->sampler);
  }

  // Rect program
  {
    // Creating a buffer for input assembler data transfer
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = Megabytes(8); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->rect_program_ia_buffer);
    }
    
    // Uniform buffer for rect program
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = sizeof(D3D_Rect_unifrom_data); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->rect_program_uniform_buffer); 
    }

    // Loading programs
    d3d->rect_program = r_program_from_file(L"../data/shaders/test_rect_shader.hlsl", "vs_main", "ps_main", __r_g_rect_program_input_assembler_element_desc, ArrayCount(__r_g_rect_program_input_assembler_element_desc));
  }

  // Texture program
  {
    // Creating a buffer for input assembler data transfer
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = Megabytes(8); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->texture_program_ia_buffer);
    }
    
    // Uniform buffer for rect program
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = sizeof(D3D_Rect_unifrom_data); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->texture_program_uniform_buffer); 
    }

    // Loading programs
    d3d->texture_program = r_program_from_file(L"../data/shaders/draw_texture_program_shader.hlsl", "vs_main", "ps_main", __r_g_texture_program_input_assembler_element_desc, ArrayCount(__r_g_texture_program_input_assembler_element_desc));
  }
}

void r_relesase()
{
  // todo:
}

// todo: Name this section here
// R_Handle r_handle_zero() 
// { 
//   R_Handle handle = {};
//   return handle;
// }

// B32 r_handle_match(R_Handle handle, R_Handle other)
// {
//   B32 is_match = (
//        handle.swap_chain           == other.swap_chain
//     && handle.frame_buffer_texture == other.frame_buffer_texture
//     && handle.frame_buffer_rtv     == other.frame_buffer_rtv
//   );
//   return is_match;
// }

void r_render_begin(R_Chain chain)
{
  // todo: Do this when handles are done and api is ready
  // if (r_handle_match(render_handle, r_handle_zero())) { return; }

  {
    B32 match = chain.__win32_window_handle_for_assert == os_get_state()->window.handle;
    if (!match) 
    {
      // note: 
      // Up to this moment the renderer uses a single window directly from the os layer, since
      // the os layer only had 1 window. At some point it was possible that you would start to use 
      // more windows, and you would have to render them. For that reason i have put a window handle into 
      // the renderer that i would then hard code at handle creation with the same window from the win32 
      // state. If you are reading this, the hi dude, hope you are doing great, making some 3s.
      // The code after this comment uses the calls for window stuff specific to that win32 window.
      // Now that you have more windows, have os supplie them via handles and then use those to get 
      // data about them to then use here.
      NotImplemented();
    }
  }

  D3D_State* d3d = r_get_state();

  V2F32 chain_dims  = r_get_swap_chain_dims(chain);
  V2F32 window_dims = os_get_window_dims();

  // Resizing the frame buffer
  if ( window_dims.x != 0.0f 
    && window_dims.y != 0.0f 
    && !v2f32_match(chain_dims, window_dims)
  ) {
    chain.texture->Release();
    chain.texture_rtv->Release();

    chain.swap_chain->ResizeBuffers(0, (UINT)os_get_window_dims().x, (UINT)os_get_window_dims().y, DXGI_FORMAT_UNKNOWN, 0);
    chain.swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&chain.texture);
    d3d->device->CreateRenderTargetView((ID3D11Resource*)chain.texture, NULL, &chain.texture_rtv);
  }
}

void r_render_end(R_Chain chain)
{
  // Nothing here, keeping this to have a logical pair of render_begin/render_end
}

///////////////////////////////////////////////////////////
// - Clearing
//
void r_clear_chain(R_Chain chain, V4F32 color)
{
  // todo: if chain is 0 return

  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(chain.texture_rtv, color.v);
}

void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color)
{
  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(rtv, color.v);
}

R_Chain r_attach_window(OS_Window window)
{
  D3D_State* d3d = r_get_state();

  HRESULT hr = S_OK;
  
  IDXGISwapChain1* swap_chain = 0;
  {
    IDXGIDevice* dxgi_device = 0;
    hr = d3d->device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
    HR(hr);

    IDXGIAdapter* dxgi_adapter = 0;
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    HR(hr);

    IDXGIFactory2* dxgi_factory = 0;
    hr = dxgi_adapter->GetParent(IID_IDXGIFactory2, (void**)&dxgi_factory);
    HR(hr);

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width       = 69;
    desc.Height      = 69;
    desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo      = FALSE;
    desc.SampleDesc  = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling     = (os_window_is_transparent() ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE);                    // todo: Learn what these do
    desc.AlphaMode   = (os_window_is_transparent() ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_UNSPECIFIED); // todo: Learn what these do
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags       = 0;
    if  (os_window_is_transparent()) { hr = dxgi_factory->CreateSwapChainForComposition(d3d->device, &desc, Null, &swap_chain); }
    else                             { hr = dxgi_factory->CreateSwapChainForHwnd(d3d->device, window.handle, &desc, Null, Null, &swap_chain); }
    HR(hr);

    dxgi_factory->Release();
    dxgi_adapter->Release();
    dxgi_device->Release();
  }

  ID3D11Texture2D* frame_buffer_texture = 0;
  {
    hr = swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&frame_buffer_texture);
    HR(hr);
  }

  ID3D11RenderTargetView* frame_buffer_rtv = 0;
  {
    hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)frame_buffer_texture, NULL, &frame_buffer_rtv);
    HR(hr);
  }

  R_Chain handle = {};
  handle.__win32_window_handle_for_assert = window.handle;
  handle.swap_chain  = swap_chain;
  handle.texture     = frame_buffer_texture;
  handle.texture_rtv = frame_buffer_rtv;
  return handle;
}

void r_submit(R_Target target, D_Command_batch_list* command_batch_list)
{
  D3D_State* d3d = r_get_state();
  V2F32 rtv_dims = r_get_target_dims(target);

  // Setting state that is the same for all batches
  {
    d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Rasterizer
    d3d->context->RSSetState(d3d->rasterizer_state);

    // Viewport 
    {
      D3D11_VIEWPORT vp = {};
      vp.TopLeftX = 0.0f;
      vp.TopLeftY = 0.0f;
      vp.Width    = rtv_dims.x;
      vp.Height   = rtv_dims.y;
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      d3d->context->RSSetViewports(1, &vp);
    }
  }

  // Working with batches
  for (D_Command_batch* batch = command_batch_list->first; batch; batch = batch->next_batch)
  {
    d3d->context->OMSetRenderTargets(1, &batch->rtv, Null);
    d3d->context->RSSetScissorRects(0, 0);
    
    if (0)                                                  {}
    else if (batch->blend_kind == D3D_Blend_kind__alpha)    { d3d->context->OMSetBlendState(d3d->alpha_blend_state, Null, ~0U); }
    else if (batch->blend_kind == D3D_Blend_kind__no_blend) { d3d->context->OMSetBlendState(Null, Null, ~0U); }

    if (batch->command_type == D_Command_type__Rect)
    {
      d3d->context->IASetInputLayout(d3d->rect_program.input_layout);

      // Filling up the uniform buffer with data 
      {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        d3d->context->Map((ID3D11Resource*)d3d->rect_program_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        D3D_Rect_unifrom_data uniform_data = {};
        uniform_data.u_window_width  = rtv_dims.x;
        uniform_data.u_window_height = rtv_dims.y;
        memcpy(mapped.pData, &uniform_data, sizeof(uniform_data));
        d3d->context->Unmap((ID3D11Resource*)d3d->rect_program_uniform_buffer, 0);
      }
      
      // Vertex shader
      d3d->context->VSSetShader(d3d->rect_program.v_shader, Null, Null);
      d3d->context->VSSetConstantBuffers(0, 1, &d3d->rect_program_uniform_buffer);  
      
      // Pixel shader
      d3d->context->PSSetShader(d3d->rect_program.p_shader, Null, Null);
      d3d->context->PSSetConstantBuffers(0, 1, &d3d->rect_program_uniform_buffer);

      // Filling up the ia buffer with data
      { 
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        // todo: This doesnt check the cap for size of the buffer, this shoud be fixed
        d3d->context->Map((ID3D11Resource*)d3d->rect_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        U64 i = 0;
        for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
        {
          D3D_Rect_instance_data instance_data = {};
          instance_data.origin_x      = node->command.u.rect_c.rect.x; 
          instance_data.origin_y      = node->command.u.rect_c.rect.y; 
          instance_data.width         = node->command.u.rect_c.rect.width;
          instance_data.height        = node->command.u.rect_c.rect.height;
          instance_data.color_00      = node->command.u.rect_c.vertex_color[UV__00];
          instance_data.color_01      = node->command.u.rect_c.vertex_color[UV__01];
          instance_data.color_10      = node->command.u.rect_c.vertex_color[UV__10];
          instance_data.color_11      = node->command.u.rect_c.vertex_color[UV__11];
          instance_data.corner_radius_00 = node->command.u.rect_c.corner_radius[UV__00];
          instance_data.corner_radius_01 = node->command.u.rect_c.corner_radius[UV__01];
          instance_data.corner_radius_10 = node->command.u.rect_c.corner_radius[UV__10];
          instance_data.corner_radius_11 = node->command.u.rect_c.corner_radius[UV__11];
          instance_data.border_color     = node->command.u.rect_c.border_color;
          instance_data.border_thickness = node->command.u.rect_c.border_thickness;
          instance_data.softness         = node->command.u.rect_c.softness;

          memcpy((D3D_Rect_instance_data*)mapped.pData + i, &instance_data, sizeof(instance_data));
        }
        d3d->context->Unmap((ID3D11Resource*)d3d->rect_program_ia_buffer, 0);
      }

      UINT stride = sizeof(D3D_Rect_instance_data);
      UINT offset = 0;
      d3d->context->IASetVertexBuffers(0, 1, &d3d->rect_program_ia_buffer, &stride, &offset);
    }
    else if (batch->command_type == D_Command_type__Texture)
    {
      d3d->context->IASetInputLayout(d3d->texture_program.input_layout);

      // Filling up the uniform buffer with data 
      {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        d3d->context->Map((ID3D11Resource*)d3d->texture_program_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        D3D_Texture_uniform_data uniform_data = {};
        uniform_data.u_window_width  = rtv_dims.x;
        uniform_data.u_window_height = rtv_dims.y;
        memcpy(mapped.pData, &uniform_data, sizeof(uniform_data));
        d3d->context->Unmap((ID3D11Resource*)d3d->texture_program_uniform_buffer, 0);
      }

      d3d->context->PSSetSamplers(0, 1, &d3d->sampler);
      
      // Vertex shader
      d3d->context->VSSetShader(d3d->texture_program.v_shader, Null, Null);
      d3d->context->VSSetConstantBuffers(0, 1, &d3d->texture_program_uniform_buffer);  
      
      // Pixel shader
      d3d->context->PSSetShader(d3d->texture_program.p_shader, Null, Null);
      d3d->context->PSSetConstantBuffers(0, 1, &d3d->texture_program_uniform_buffer);
      
      {
        ID3D11ShaderResourceView* texture_view = 0;
        d3d->device->CreateShaderResourceView((ID3D11Resource*)batch->texture, NULL, &texture_view);
        d3d->context->PSSetShaderResources(0, 1, &texture_view);
        texture_view->Release();
      }

      // Filling up the ia buffer with data
      {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        d3d->context->Map((ID3D11Resource*)d3d->texture_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        U64 i = 0;
        for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
        {
          D3D_Texture_instance_data instance_data = {};
          instance_data.dest_rect_origin = rect_get_origin(node->command.u.texture_c.dest_rect);
          instance_data.dest_rect_size   = rect_get_dims(node->command.u.texture_c.dest_rect);
          instance_data.src_rect_origin  = rect_get_origin(node->command.u.texture_c.src_rect);
          instance_data.src_rect_size    = rect_get_dims(node->command.u.texture_c.src_rect);
          instance_data.src_texture_dims = r_get_texture_dims(batch->texture);
          instance_data.is_text_texture  = node->command.u.texture_c.is_text;
          instance_data.text_color       = node->command.u.texture_c.text_color;
          memcpy((D3D_Texture_instance_data*)mapped.pData + i, &instance_data, sizeof(instance_data));
        }
        d3d->context->Unmap((ID3D11Resource*)d3d->texture_program_ia_buffer, 0);
      }

      UINT stride = sizeof(D3D_Texture_instance_data);
      UINT offset = 0;
      d3d->context->IASetVertexBuffers(0, 1, &d3d->texture_program_ia_buffer, &stride, &offset);
    }
    else { InvalidCodePath(); }
  
    d3d->context->DrawInstanced(4, (UINT)batch->count, 0, 0);
  }
}

void r_present_swap_chain(R_Chain swap_chain, B32 vsync)
{
  swap_chain.swap_chain->Present(!!vsync, 0);
  // HRESULT commit_hr = comp_device->Commit(); 
  // Handle(commit_hr == S_OK);
}

///////////////////////////////////////////////////////////
// - Other
//
// note: This makes a texture that is for rendering into and rendering with
ID3D11Texture2D* r_make_texture(U32 width, U32 height)
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
  return texture;
}

ID3D11RenderTargetView* r_rtv_from_texture(ID3D11Texture2D* texture)
{
  D3D_State* d3d = r_get_state();
  ID3D11RenderTargetView* rtv = 0;
  HRESULT hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &rtv);
  Handle(hr == S_OK);
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

V2F32 r_get_texture_dims(ID3D11Texture2D* texture)
{
  D3D11_TEXTURE2D_DESC desc = {};
  texture->GetDesc(&desc);
  return v2f32((F32)desc.Width, (F32)desc.Height);
}

// note: Returns D3D_Program{} if fails
D3D_Program r_program_from_file(const WCHAR* shader_program_file, 
                                const char* v_shader_main_f_name, 
                                const char* p_shader_main_f_name, 
                                const D3D11_INPUT_ELEMENT_DESC* opt_desc_arr,
                                U32 desc_arr_count
) {
  D3D_State* d3d = r_get_state();

  UINT flags = 0;
  #if DEBUG_MODE
  flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  #endif

  // V shader compilation
  ID3DBlob* v_blob = 0;
  {
    ID3DBlob* error_blob = 0;

    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "vs_main", "vs_5_0", flags, Null, &v_blob, &error_blob
    );

    if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
    if (error_blob != 0) { error_blob->Release(); }
  }

  // P shader compilation
  ID3DBlob* p_blob = 0;
  {
    ID3DBlob* error_blob = 0;
  
    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "ps_main", "ps_5_0", flags, Null, &p_blob, &error_blob
    );

    if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3D11VertexShader* v_shader    = 0;
  ID3D11PixelShader* p_shader     = 0;
  ID3D11InputLayout* input_layout = 0;
  {
    d3d->device->CreateVertexShader(v_blob->GetBufferPointer(), v_blob->GetBufferSize(), Null, &v_shader);
    d3d->device->CreatePixelShader(p_blob->GetBufferPointer(), p_blob->GetBufferSize(), Null, &p_shader);
    if (opt_desc_arr != 0)
    {
      d3d->device->CreateInputLayout(opt_desc_arr, desc_arr_count, v_blob->GetBufferPointer(), v_blob->GetBufferSize(), &input_layout);
    }
  }

  if (p_blob != 0) { p_blob->Release(); }  
  if (v_blob != 0) { v_blob->Release(); }  

  D3D_Program program = {};
  program.v_shader     = v_shader;
  program.p_shader     = p_shader;
  program.input_layout = input_layout;
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
  int succ = stbi_write_png((char*)file_path_nt.data, (int)image.width_in_px, (int)image.height_in_px, (int)image.bytes_per_pixel, image.data, (int)(image.width_in_px * image.bytes_per_pixel));
  Handle(succ);
  end_scratch(&scratch);
}

ID3D11Texture2D* r_load_texture_from_file(Str8 file_name)
{
  Scratch scratch = get_scratch(0, 0);
  D3D_State* d3d = r_get_state();
  ID3D11Texture2D* result_texture = 0;

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
    result_texture = r_load_texture_from_image(image);
  }

  end_scratch(&scratch);
  return result_texture;
}

ID3D11Texture2D* r_load_texture_from_image(Image image)
{
  D3D_State* d3d = r_get_state();
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
  
  return texture;
}

// note: This is a very shitty function ))
void r_copy_into_texture_from_texture(
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

V2F32 r_get_swap_chain_dims(R_Chain chain)
{
  // todo: return if chain i n0

  V2F32 dims = {};
  DXGI_SWAP_CHAIN_DESC1 desc = {};
  if (chain.swap_chain->GetDesc1(&desc) == S_OK)
  {
    dims = v2f32((F32)desc.Width, (F32)desc.Height);
  }
  return dims;
}

V2F32 r_get_target_dims(R_Target target)
{
  // todo: return if target is 0
  D3D11_TEXTURE2D_DESC desc = {};
  target.texture->GetDesc(&desc);
  return v2f32((F32)desc.Width, (F32)desc.Height);
}

R_Target r_target_from_swap_chain(R_Chain chain)
{
  // todo: If chain is 00 return
  R_Target target = {};
  target.texture     = chain.texture;
  target.texture_rtv = chain.texture_rtv;
  return target;
}

#undef HR

#endif