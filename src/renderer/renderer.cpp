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

const global 
D3D11_INPUT_ELEMENT_DESC rect_program_input_assembler_element_desc[] = 
{
  { "RECT_COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(D3D_Rect_instance_data, color),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_X", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, origin_x), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_Y", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, origin_y), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_WIDTH",    0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, width),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_HEIGHT",   0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(D3D_Rect_instance_data, height),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

D3D11_INPUT_ELEMENT_DESC texture_program_input_assembler_element_desc[] = 
{
  { "DEST_RECT_ORIGIN", 0, DXGI_FORMAT_R32G32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, dest_rect_origin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_SIZE",   0, DXGI_FORMAT_R32G32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, dest_rect_size),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_ORIGIN",  0, DXGI_FORMAT_R32G32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, src_rect_origin),  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_SIZE",    0, DXGI_FORMAT_R32G32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, src_rect_size),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_TEXTURE_DIMS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, TypeFieldOffset(D3D_Texture_instance_data, src_texture_dims), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

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
    UINT flags = 0;
    #if DEBUG_MODE
    flags = D3D11_CREATE_DEVICE_DEBUG; 
    #endif

    hr = D3D11CreateDevice(
      dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN /*D3D_DRIVER_TYPE_HARDWARE*/, Null,  
      flags, levels, ArrayCount(levels),
      D3D11_SDK_VERSION, &d3d->device, Null, &d3d->context
    );
    HR(hr);
  }
  ID3D11Device* d3d_device = d3d->device;
  ID3D11DeviceContext* d3d_context = d3d->context;   

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

  // Frame buffer
  {
    d3d->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&d3d->frame_buffer_texture);
    d3d->device->CreateRenderTargetView((ID3D11Resource*)d3d->frame_buffer_texture, NULL, &d3d->frame_buffer_rtv);
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
    d3d->rect_program = r_program_from_file(L"../data/shaders/test_rect_shader.hlsl", "vs_main", "ps_main", rect_program_input_assembler_element_desc, ArrayCount(rect_program_input_assembler_element_desc));
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
    d3d->texture_program = r_program_from_file(L"../data/shaders/draw_texture_program_shader.hlsl", "vs_main", "ps_main", texture_program_input_assembler_element_desc, ArrayCount(texture_program_input_assembler_element_desc));
  }

  #undef HR // todo: Remove this HR shit from here
}

void r_relesase()
{
  // todo:
}

///////////////////////////////////////////////////////////
// - TESTING THE ERROR A NEW HANDLING WAY FOR THE LAYER
//
// void __r_push_error_message(Str8 str)
// {
//   D3D_State* d3d = r_get_state();
//   Str8 error_str = str8_copy_alloc(d3d->debug_arena, str);
//   d3d->error_messages_count += 1;
// }
// #define __R_PUSH_ERROR_AND_DEBUG(c_lit) do { __r_push_error_message(Str8FromC(c_lit)); BreakPoint(); } while (0); 
///////////////////////////////////////////////////////////

void r_render_begin(V2F32 vp_dims)
{
  D3D_State* d3d = r_get_state();

  // Resizing the frame buffer
  if (vp_dims.x != 0.0f && vp_dims.y != 0.0f)
  {
    V2F32 window_dims = os_get_client_area_dims();
    d3d->frame_buffer_texture->Release();
    d3d->frame_buffer_rtv->Release();
    d3d->swap_chain->ResizeBuffers(0, (UINT)window_dims.x, (UINT)window_dims.y, DXGI_FORMAT_UNKNOWN, 0);
    d3d->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&d3d->frame_buffer_texture);
    d3d->device->CreateRenderTargetView((ID3D11Resource*)d3d->frame_buffer_texture, NULL, &d3d->frame_buffer_rtv);
  }
}

void r_render_end()
{
  // Nothing here, keeping this to have a logical pair of render_begin/render_end
}

///////////////////////////////////////////////////////////
// - Clearing
//
void r_clear_frame_buffer(V4F32 color)
{
  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(d3d->frame_buffer_rtv, color.v);
}

void r_clear_rtv(ID3D11RenderTargetView* rtv, V4F32 color)
{
  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(rtv, color.v);
}

///////////////////////////////////////////////////////////
// - New drawing
//
/*
void r_draw_text(Str8 text, V2F32 pos, FP_Font font, V4F32 color)
{
  // note: These are some debug drawings for baseline and stuff
  // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y, 100, 1), green_f());
  // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y + font.ascent + font.descent, 100, 1), green_f());
  // r_draw_rect(dest_rtv, rect_make(pos.x, origin_y, 100, 1), green_f());
  
  F32 origin_y = pos.y + font.ascent;
  F32 x_offset = 0.0f;

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
*/
// todo: This call gets all the batches and then submits them to be drawn by the gpu to the rendering target
void r_submit(D_Command_batch_list* command_batch_list)
{
  D3D_State* d3d = r_get_state();
  V2F32 rtv_dims = r_get_texture_dims(d3d->frame_buffer_texture);

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

    d3d->context->OMSetRenderTargets(1, &d3d->frame_buffer_rtv, Null);
    d3d->context->OMSetBlendState(d3d->alpha_blend_state, Null, ~0U);
  }

  // Working with batches
  for (D_Command_batch* batch = command_batch_list->first; batch; batch = batch->next_batch)
  {
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
        d3d->context->Map((ID3D11Resource*)d3d->rect_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        U64 i = 0;
        for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
        {
          D3D_Rect_instance_data instance_data = {};
          instance_data.origin_x = node->command.u.rect_c.rect.x; 
          instance_data.origin_y = node->command.u.rect_c.rect.y; 
          instance_data.width    = node->command.u.rect_c.rect.width;
          instance_data.height   = node->command.u.rect_c.rect.height;
          instance_data.color    = node->command.u.rect_c.color;
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
          instance_data.dest_rect_origin = rect_origin(node->command.u.texture_c.dest_rect);
          instance_data.dest_rect_size   = rect_dims(node->command.u.texture_c.dest_rect);
          instance_data.src_rect_origin  = rect_origin(node->command.u.texture_c.src_rect);
          instance_data.src_rect_size    = rect_dims(node->command.u.texture_c.src_rect);
          instance_data.src_texture_dims = r_get_texture_dims(batch->texture);
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

///////////////////////////////////////////////////////////
// - Old drawing code
//

// void r_draw_texture(ID3D11RenderTargetView* dest_rtv, Rect rect_in_dest, ID3D11RenderTargetView* src_rtv, Rect rect_in_src)
// {
//   D3D_State* d3d = r_get_state();
//   HRESULT hr = S_OK;

//   // Setting the uniform buffer
//   {
//     D3D_Texture_program_data u_data = {};
//     u_data.vp_width         = d3d->render_viewport_width;
//     u_data.vp_height        = d3d->render_viewport_height;
//     u_data.dest_rect_origin = v2f32(rect_in_dest.x, rect_in_dest.y);
//     u_data.dest_rect_size   = v2f32(rect_in_dest.width, rect_in_dest.height);
//     u_data.src_rect_origin  = v2f32(rect_in_src.x, rect_in_src.y);
//     u_data.src_rect_size    = v2f32(rect_in_src.width, rect_in_src.height);
//     u_data.src_texture_dims = r_get_texture_dims(src_rtv);

//     D3D11_MAPPED_SUBRESOURCE mapped = {};
//     hr = d3d->context->Map((ID3D11Resource*)d3d->uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
//     InvariantCheck(hr == S_OK);
//     if (hr == S_OK) {
//       memcpy(mapped.pData, &u_data, sizeof(u_data));
//       d3d->context->Unmap((ID3D11Resource*)d3d->uniform_buffer, 0);
//     } 
//   }

//   d3d->context->VSSetShader(d3d->draw_texture_program.v_shader, 0, 0);
//   d3d->context->VSSetConstantBuffers(0, 1, &d3d->uniform_buffer);

//   ID3D11SamplerState* sampler;
//   {
//     D3D11_SAMPLER_DESC desc = {};
//     desc.Filter        = D3D11_FILTER_MIN_MAG_MIP_POINT;
//     desc.AddressU      = D3D11_TEXTURE_ADDRESS_WRAP;
//     desc.AddressV      = D3D11_TEXTURE_ADDRESS_WRAP;
//     desc.AddressW      = D3D11_TEXTURE_ADDRESS_WRAP;
//     desc.MipLODBias    = 0;
//     desc.MaxAnisotropy = 1;
//     desc.MinLOD        = 0;
//     desc.MaxLOD        = D3D11_FLOAT32_MAX;
//     d3d->device->CreateSamplerState(&desc, &sampler);
//   }
//   d3d->context->PSSetSamplers(0, 1, &sampler);
//   d3d->context->PSSetShader(d3d->draw_texture_program.p_shader, 0, 0);
//   {
//     ID3D11Resource* resource = 0;
//     src_rtv->GetResource(&resource);

//     ID3D11ShaderResourceView* view = 0;
//     d3d->device->CreateShaderResourceView(resource, NULL, &view);
//     d3d->context->PSSetShaderResources(0, 1, &view);
//     d3d->context->PSSetConstantBuffers(0, 1, &d3d->uniform_buffer);
//     view->Release();
//     resource->Release();
//   }

//   d3d->context->OMSetRenderTargets(1, &dest_rtv, 0);

//   d3d->context->Draw(4, 0);

//   sampler->Release();
// }

// #include "pencil/pencil.h"

// void r_draw_text(ID3D11RenderTargetView* dest_rtv, V2F32 pos, FP_Font font, V4F32 color, Str8 text)
// {
//   ProfileFuncBegin();
  
//   F32 origin_y = pos.y + font.ascent;
//   F32 x_offset = 0.0f;
  
//   // note: These are some debug drawings for baseline and stuff
//   // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y, 100, 1), green_f());
//   // r_draw_rect(dest_rtv, rect_make(pos.x, pos.y + font.ascent + font.descent, 100, 1), green_f());
//   // r_draw_rect(dest_rtv, rect_make(pos.x, origin_y, 100, 1), green_f());

//   for (U64 ch_index = 0; ch_index < text.count; ch_index += 1)
//   {
//     U8 ch = text.data[ch_index];
//     FP_Codepoint_data glyph_data = fp_get_glyph_data(font, ch); 

//     F32 origin_x = pos.x + x_offset;

//     // Just puttin them 1 next to another
//     Rect dest_rect = {};
//     dest_rect.x      = origin_x + glyph_data.bearing_x;
//     dest_rect.y      = origin_y - glyph_data.bearing_y;
//     dest_rect.width  = glyph_data.rect_on_atlas.width;
//     dest_rect.height = glyph_data.rect_on_atlas.height;
    
//     r_draw_texture(
//       dest_rtv, dest_rect,
//       font.atlas_texture, glyph_data.rect_on_atlas
//     );

//     F32 advance = glyph_data.advance;
//     if (ch_index < text.count - 1)
//     {
//       FP_Kerning_entry entry = fp_get_kerning(font, ch, text.data[ch_index + 1]);
//       if (!IsMemZero(entry)) { advance += entry.advance; }
//     } 
//     x_offset += advance; 
//   }

//   ProfileFuncEnd();
// }

// void r_draw_text_f(ID3D11RenderTargetView* dest_rtv, V2F32 pos, FP_Font font, V4F32 color, const char* fmt, ...)
// {
//   // todo: This only uses 128
//   va_list argptr;
//   va_start(argptr, fmt);
//   Scratch scratch = get_scratch(0, 0);
//   U64 buffer_count = 128;
//   U8* buffer = ArenaPushArr(scratch.arena, U8, buffer_count);
//   int err = vsnprintf((char*)buffer, buffer_count, fmt, argptr);
//   if (err < 0) { Assert(0); }
//   else if (err >= buffer_count) { Assert(0); }
//   else if (err < buffer_count) { /* All good */ }
//   va_end(argptr);
//   Str8 str = str8_manuall(buffer, (U64)err);
//   r_draw_text(dest_rtv, pos, font, color, str);
//   end_scratch(&scratch);
// }

///////////////////////////////////////////////////////////
// - Other
//
ID3D11RenderTargetView* r_get_frame_buffer_rtv()
{
  return r_get_state()->frame_buffer_rtv;
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

// V2F32 r_get_texture_dims(ID3D11RenderTargetView* rtv)
// {
//   D3D_Texture_result texture_res = r_texture_from_rtv(rtv);
//   Handle(texture_res.succ);

//   D3D11_TEXTURE2D_DESC desc = {};
//   texture_res.texture->GetDesc(&desc);

//   texture_res.texture->Release();
//   V2F32 dims = v2f32((F32)desc.Width, (F32)desc.Height);
//   return dims;
// }

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

  // V shader compilation
  ID3DBlob* v_blob = 0;
  {
    ID3DBlob* error_blob = 0;

    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "vs_main", "vs_5_0", Null, Null, &v_blob, &error_blob
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
      "ps_main", "ps_5_0", Null, Null, &p_blob, &error_blob
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

void r_scissoring_set(Rect rect)
{
  D3D_State* d3d = r_get_state();

  // Release these at the bottom
  ID3D11RasterizerState* old_rasterizer_state = 0;
  ID3D11RasterizerState* new_rasterizer_state = 0;
  
  D3D11_RASTERIZER_DESC desc = {};
  d3d->context->RSGetState(&old_rasterizer_state);
  old_rasterizer_state->GetDesc(&desc);
  desc.ScissorEnable = true;
  
  HRESULT hr = d3d->device->CreateRasterizerState(&desc, &new_rasterizer_state);
  Handle(hr == S_OK);
  
  d3d->context->RSSetState(new_rasterizer_state);
  
  D3D11_RECT scissorRect = {};
  scissorRect.left   = (LONG)(rect.x);
  scissorRect.top    = (LONG)(rect.y);
  scissorRect.right  = (LONG)(rect.x + rect.width);
  scissorRect.bottom = (LONG)(rect.y + rect.height);
  d3d->context->RSSetScissorRects(1, &scissorRect);
  
  new_rasterizer_state->Release();
  old_rasterizer_state->Release();
}

void r_scissoring_clear()
{
  D3D_State* d3d = r_get_state();

  ID3D11RasterizerState* old_rasterizer_state = 0;
  ID3D11RasterizerState* new_rasterizer_state = 0;

  D3D11_RASTERIZER_DESC desc = {};
  d3d->context->RSGetState(&old_rasterizer_state);
  old_rasterizer_state->GetDesc(&desc);
  desc.ScissorEnable = false;

  HRESULT hr = d3d->device->CreateRasterizerState(&desc, &new_rasterizer_state);
  Handle(hr == S_OK);
  
  d3d->context->RSSetState(new_rasterizer_state);
  d3d->context->RSSetScissorRects(0, 0);
  
  new_rasterizer_state->Release();
  old_rasterizer_state->Release();
}

#endif