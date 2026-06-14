#ifndef RENDERER_D3D11_CPP
#define RENDERER_D3D11_CPP

// todos:
// [] - Do we need the d3d context to switch for every batch or would it be fine for it to only switch the setting.

// D3D
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")      // This is for the ids for the all the interfaces
#pragma comment (lib, "d3dcompiler.lib")

// DWM
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "dcomp.lib")

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "render.h"

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
  for EachEnumRange(fill_mode, R_Fill_mode, R_Fill_mode__solid, R_Fill_mode__COUNT)
  {
    ID3D11RasterizerState** raster_state = d3d->rasterizer_states + fill_mode;
    D3D11_FILL_MODE d3d_fill_mode = D3D11_FILL_SOLID;
    switch(fill_mode)
    {
      case R_Fill_mode__solid:     { d3d_fill_mode = D3D11_FILL_SOLID;     } break;
      case R_Fill_mode__wireframe: { d3d_fill_mode = D3D11_FILL_WIREFRAME; } break;
      default: { InvalidCodePath(); } break;
    }

    // No culling, this makes all the triangles appear, not only the once that follow a specific clock direction (meaning clock-wise or counter clock-wise)
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode        = d3d_fill_mode;
    desc.CullMode        = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    desc.ScissorEnable   = true;
    hr = d3d->device->CreateRasterizerState(&desc, raster_state);
    HR(hr);
  }

  // Blending states (Here is a link to some blending modes and their formulas: https://developer.android.com/reference/kotlin/android/graphics/PorterDuff.Mode)
  {
    // Alpha blend
    {
      // todo: Have this be a real alpha blend with the alpha chanell not squared and see the diff for the new version with the old version
      D3D11_BLEND_DESC desc = {};
      desc.RenderTarget[0].BlendEnable           = TRUE;
      desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      d3d->device->CreateBlendState(&desc, &d3d->blend_states[R_Blend_kind__alpha]);
    }

    // No blend
    {
      D3D11_BLEND_DESC desc = {};
      desc.RenderTarget[0].BlendEnable = FALSE;
      desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; 
      d3d->device->CreateBlendState(&desc, &d3d->blend_states[R_Blend_kind__no_blend]);
    }
  
    // Dest out (Keeps the destination pixels that are not covered by source pixels. Discards destination pixels that are covered by source pixels. Discards all source pixels.)
    {
      D3D11_BLEND_DESC desc = {};
      desc.RenderTarget[0].BlendEnable           = TRUE;
      desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ZERO;
      desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
      desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      d3d->device->CreateBlendState(&desc, &d3d->blend_states[R_Blend_kind__dest_out]);
    }
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

  // // Magenta black texture
  // {
  //   D3D11_TEXTURE2D_DESC desc = {};
  //   desc.Width      = 2;
  //   desc.Height     = 2;
  //   desc.MipLevels  = 1;
  //   desc.ArraySize  = 1;
  //   desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
  //   desc.SampleDesc = { 1, 0 };
  //   desc.Usage      = D3D11_USAGE_DEFAULT;
  //   desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE; 
    
  //   V4U8 texture_data[4] = {
  //     black_u(),   magenta_u(),
  //     magenta_u(), black_u(),
  //   };

  //   D3D11_SUBRESOURCE_DATA subresource_data = {};
  //   subresource_data.pSysMem          = (void*)texture_data;
  //   subresource_data.SysMemPitch      = 2 * sizeof(V4U8);
  //   subresource_data.SysMemSlicePitch = Null;

  //   d3d->device->CreateTexture2D(&desc, &subresource_data, &d3d->magenta_black_d3d_texture);
  // }

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
      desc.ByteWidth      = sizeof(R_Rect_unifrom_data); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->rect_program_uniform_buffer); 
    }

    // Loading programs
    d3d->rect_program = r_program_from_file(L"../data/shaders/rect_shader_for_ui.hlsl", "vs_main", "ps_main", __r_g_rect_program_input_assembler_element_desc, ArrayCount(__r_g_rect_program_input_assembler_element_desc));
  }

  // todo: These programm stuff shoud be made a loop with an enum to index into them and then a loop to compile
  //       and set data for them here, cause this is getting out of hand and alos error prone.

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
    
    // Uniform buffer for texture program
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = sizeof(R_Texture_uniform_data); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->texture_program_uniform_buffer); 
    }

    // Loading programs
    d3d->texture_program = r_program_from_file(L"../data/shaders/texture_shader.hlsl", "vs_main", "ps_main", __r_g_texture_program_input_assembler_element_desc, ArrayCount(__r_g_texture_program_input_assembler_element_desc));
  }

  // Line program
  {
    // Creating a buffer for input assembler data transfer
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = Megabytes(8); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->line_program_ia_buffer);
    }
    
    // Uniform buffer for line program
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth      = sizeof(R_Line_uniform_data); 
      desc.Usage          = D3D11_USAGE_DYNAMIC; // Dynamic is for for gpu to read and for cpu to write 
      desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      d3d->device->CreateBuffer(&desc, 0, &d3d->line_program_uniform_buffer); 
    }

    // Loading programs
    d3d->line_program = r_program_from_file(L"../data/shaders/line_shader.hlsl", "vs_main", "ps_main", __r_g_line_program_input_assembler_element_desc, ArrayCount(__r_g_line_program_input_assembler_element_desc));
  }
}

void r_relesase()
{
  // todo:
}

///////////////////////////////////////////////////////////
// - Rendering work flow 
//
R_Target r_attach_window(OS_Window window)
{
  // todo: Check if window is not zero here when you start having them

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
    hr = d3d->device->CreateRenderTargetView(frame_buffer_texture, NULL, &frame_buffer_rtv);
    HR(hr);
  }

  // Here is the link to the resource that explaince this code here and why it is needed
  // https://learn.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine
  // todo: Do better with this here
  IDCompositionDevice* comp_device = 0;
  if (os_window_is_transparent())
  {
    IDXGIDevice* dxgi_device    = 0;
    IDCompositionVisual* visual = 0;
    IDCompositionTarget* target = 0;

    hr = d3d->device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
    HR(hr);
    
    hr = DCompositionCreateDevice(dxgi_device, __uuidof(comp_device), (void**)&comp_device);
    HR(hr);

    hr = comp_device->CreateVisual(&visual);
    HR(hr);
  
    hr = visual->SetContent((IUnknown*)swap_chain);
    HR(hr);
    
    hr = comp_device->CreateTargetForHwnd(window.handle, true, &target);
    HR(hr);

    hr = target->SetRoot(visual);
    HR(hr);

    // todo: If i release there (at least some of therse the ->commit dont work, look into this)
    //       when you done with other stuff, for now its fine
    // target->Release();
    // visual->Release();
    // dxgi_device->Release();
  }

  R_Target handle = {};
  handle.__win32_window_handle_for_assert = window.handle;
  handle.swap_chain  = swap_chain;
  handle.comp_device = comp_device;
  handle.texture     = frame_buffer_texture;
  handle.texture_rtv = frame_buffer_rtv;
  return handle;
}

void r_prepare_canvas(R_Target* chain)
{
  if (!__r_is_target_valid_target_chain(*chain)) { BP; return; }
  {
    B32 match = chain->__win32_window_handle_for_assert == os_get_state()->window.handle;
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

  V2F32 chain_dims  = r_get_target_dims(*chain);
  V2F32 window_dims = os_get_client_area_dims();

  // Resizing the frame buffer
  if ( window_dims.x != 0.0f 
    && window_dims.y != 0.0f 
    && !v2f32_match(chain_dims, window_dims)
  ) {
    chain->texture->Release();
    chain->texture_rtv->Release();
    chain->texture = 0;
    chain->texture_rtv = 0;

    chain->swap_chain->ResizeBuffers(0, (UINT)window_dims.x, (UINT)window_dims.y, DXGI_FORMAT_UNKNOWN, 0);
    chain->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&chain->texture);
    d3d->device->CreateRenderTargetView(chain->texture, NULL, &chain->texture_rtv);
  }
}

// todo: This should not have to use a target here, since this implies that we are rendering into something like a frame buffer,
//       when in reality we have already made commands to the draw layer that knows into what we have to render, so this shoud just
//       render that in. 
//       Right now d_begin_batching takes in a target for a frame buffer or rather for the initial target, the one that we use as 
//       the default target. We might have to handle a null handle there and be fine or just have it still be specified, but then hav 
//       the frame buffer not passed in here like this. This really has nothing to do with the frame buffer directly, other than the fact
//       that i render into it in the end.
//       -- When fixing this api, go int othe draw layer and see what the target that is passed in there is used for to know the 
//       data flow better and not change this with no detailed knowlage about the depending system.
void r_submit(R_Target target, D_Command_batch_list* command_batch_list)
{
  if (!__r_is_target_valid_target(target)) { BP; return; }

  D3D_State* d3d = r_get_state();
  V2F32 rtv_dims = r_get_target_dims(target);

  // Setting state that is the same for all batches
  {
    d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

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
    d3d->context->OMSetRenderTargets(1, &batch->target.texture_rtv, Null);
    d3d->context->RSSetState(d3d->rasterizer_states[batch->fill_mode]);
    d3d->context->OMSetBlendState(d3d->blend_states[batch->blend_kind], Null, ~0U);
    {
      D3D11_RECT scissor_rect = {};
      scissor_rect.left   = (S32)batch->scissor_rect.x;
      scissor_rect.top    = (S32)batch->scissor_rect.y;
      scissor_rect.right  = (S32)batch->scissor_rect.x + (S32)batch->scissor_rect.width;
      scissor_rect.bottom = (S32)batch->scissor_rect.y + (S32)batch->scissor_rect.height;
      d3d->context->RSSetScissorRects(1, &scissor_rect); 
    }

    if (batch->command_type == D_Command_type__Rect)
    {
      d3d->context->IASetInputLayout(d3d->rect_program.input_layout);

      // Filling up the uniform buffer with data 
      {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        d3d->context->Map(d3d->rect_program_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        R_Rect_unifrom_data uniform_data = {};
        uniform_data.u_window_width  = rtv_dims.x;
        uniform_data.u_window_height = rtv_dims.y;
        memcpy(mapped.pData, &uniform_data, sizeof(uniform_data));
        d3d->context->Unmap(d3d->rect_program_uniform_buffer, 0);
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
        d3d->context->Map(d3d->rect_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        U64 i = 0;
        for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
        {
          R_Rect_instance_data instance_data = {};
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

          memcpy((R_Rect_instance_data*)mapped.pData + i, &instance_data, sizeof(instance_data));
        }
        d3d->context->Unmap(d3d->rect_program_ia_buffer, 0);
      }

      UINT stride = sizeof(R_Rect_instance_data);
      UINT offset = 0;
      d3d->context->IASetVertexBuffers(0, 1, &d3d->rect_program_ia_buffer, &stride, &offset);
    }
    else if (batch->command_type == D_Command_type__Texture)
    {
      if (__r_is_target_valid_target(batch->texture))
      {
        d3d->context->IASetInputLayout(d3d->texture_program.input_layout);
  
        // Filling up the uniform buffer with data 
        {
          D3D11_MAPPED_SUBRESOURCE mapped = {};
          d3d->context->Map(d3d->texture_program_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
          R_Texture_uniform_data uniform_data = {};
          uniform_data.u_window_width  = rtv_dims.x;
          uniform_data.u_window_height = rtv_dims.y;
          memcpy(mapped.pData, &uniform_data, sizeof(uniform_data));
          d3d->context->Unmap(d3d->texture_program_uniform_buffer, 0);
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
          d3d->device->CreateShaderResourceView(batch->texture.texture, NULL, &texture_view);
          d3d->context->PSSetShaderResources(0, 1, &texture_view);
          texture_view->Release();
        }
  
        // Filling up the ia buffer with data
        {
          D3D11_MAPPED_SUBRESOURCE mapped = {};
          d3d->context->Map(d3d->texture_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
          
          U64 i = 0;
          for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
          {
            R_Texture_instance_data instance_data = {};
            instance_data.dest_rect_origin = node->command.u.texture_c.dest_rect.origin; 
            instance_data.dest_rect_size   = node->command.u.texture_c.dest_rect.dims; 
            instance_data.src_rect_origin  = node->command.u.texture_c.src_rect.origin; 
            instance_data.src_rect_size    = node->command.u.texture_c.src_rect.dims;
            instance_data.src_texture_dims = r_get_target_dims(batch->texture);
            instance_data.tint             = node->command.u.texture_c.tint;
            memcpy((R_Texture_instance_data*)mapped.pData + i, &instance_data, sizeof(instance_data));
          }
          d3d->context->Unmap(d3d->texture_program_ia_buffer, 0);
        }
  
        UINT stride = sizeof(R_Texture_instance_data);
        UINT offset = 0;
        d3d->context->IASetVertexBuffers(0, 1, &d3d->texture_program_ia_buffer, &stride, &offset);
      }
    }
    else if (batch->command_type == D_Command_type__Line)
    {
      d3d->context->IASetInputLayout(d3d->line_program.input_layout);

      // Filling up the uniform buffer with data 
      {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        d3d->context->Map(d3d->line_program_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        R_Line_uniform_data uniform_data = {};
        uniform_data.viewport_width  = rtv_dims.x;
        uniform_data.viewport_height = rtv_dims.y;
        memcpy(mapped.pData, &uniform_data, sizeof(uniform_data));
        d3d->context->Unmap(d3d->line_program_uniform_buffer, 0);
      }
      
      // Vertex shader
      d3d->context->VSSetShader(d3d->line_program.v_shader, Null, Null);
      d3d->context->VSSetConstantBuffers(0, 1, &d3d->line_program_uniform_buffer);  
      
      // Pixel shader
      d3d->context->PSSetShader(d3d->line_program.p_shader, Null, Null);
      d3d->context->PSSetConstantBuffers(0, 1, &d3d->line_program_uniform_buffer);

      // Filling up the ia buffer with data
      { 
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        // todo: This doesnt check the cap for size of the buffer, this shoud be fixed
        d3d->context->Map(d3d->line_program_ia_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        U64 i = 0;
        for (D_Command_node* node = batch->first_command_node; node; node = node->next, i += 1)
        {
          R_Line_instance_data instance_data = {};
          instance_data.color = node->command.u.line_c.color; 

          memcpy((R_Line_instance_data*)mapped.pData + i, &instance_data, sizeof(instance_data));
        }
        d3d->context->Unmap(d3d->line_program_ia_buffer, 0);
      }

      UINT stride = sizeof(R_Line_instance_data);
      UINT offset = 0;
      d3d->context->IASetVertexBuffers(0, 1, &d3d->line_program_ia_buffer, &stride, &offset);
    }
    else { InvalidCodePath(); }
  
    d3d->context->DrawInstanced(4, (UINT)batch->count, 0, 0);
  }

  d3d->context->ClearState();
}

void r_present(R_Target target, B32 vsync)
{
  if (!__r_is_target_valid_target_chain(target)) { BP; return; }
  
  target.swap_chain->Present(!!vsync, 0);
  if (os_window_is_transparent()) {
    target.comp_device->Commit();
  }
}

///////////////////////////////////////////////////////////
// - Texture stuff
//
R_Target r_make_texture(U32 width, U32 height)
{
  D3D_State* d3d = r_get_state();

  ID3D11Texture2D* texture = 0;
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
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
    
    // note: Not sure if i have to do this, but it says in the docs before we first draw into the texture the data in the texture
    //       is undefined. I would like to expect it to just be all 0s. This is for that.
    //       Thought it seems that it still just puts 0s in there, but i will still keep this since it says that its undefined.
    //       --
    //       It did help after all. When i was uploading a texture to the cpu and had to have it for staging and then be read,
    //       it would have some weird couple of pixels that would have some brownish color in the top left. After adding the 
    //       default data myself that stopped.
    Handle(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM); // note: Only this is supported right now
    Data_buffer buffer = data_buffer_make(scratch.arena, width * height * 4); 

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem          = buffer.data;
    data.SysMemPitch      = width * 4;
    data.SysMemSlicePitch = Null; // note: Not used for 2d textures, only used for 3d textures

    d3d->device->CreateTexture2D(&desc, &data, &texture);
  }

  ID3D11RenderTargetView* rtv = 0;
  d3d->device->CreateRenderTargetView(texture, 0, &rtv);

  // On fail d3d will keep those pointer at 0
  R_Target handle = {};
  handle.texture     = texture;
  handle.texture_rtv = rtv;
  return handle;
}

void r_release_texture(R_Target* texture)
{
  if (!__r_is_target_valid_target(*texture)) { BP; return; }

  texture->texture_rtv->Release();
  texture->texture->Release();
  texture->texture = 0;
  texture->texture_rtv = 0;
}

///////////////////////////////////////////////////////////
// - Misc
//
// note: Returns D3D_Program{} if fails
R_Program r_program_from_file(const WCHAR* shader_program_file, 
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

  R_Program program = {};
  program.v_shader     = v_shader;
  program.p_shader     = p_shader;
  program.input_layout = input_layout;
  return program;
}

// note: Not sure how i feel about this, i would rather maybe have this be in the queue with batching and not just here like this,
//       kind of makes it harder to track when you draw and when you blit
void r_clear_target(R_Target target, V4F32 color)
{
  if (!__r_is_target_valid_target(target)) { BP; return; }
  
  D3D_State* d3d = r_get_state();
  d3d->context->ClearRenderTargetView(target.texture_rtv, color.v);
}

Image r_image_from_texture(Arena* arena, R_Target texture)
{
  if (!__r_is_target_valid_target(texture)) { BP; return Image{}; }

  D3D_State* d3d = r_get_state();
  HRESULT hr = S_OK;

  // Stuff to clear at the end
  Scratch          scratch      = get_scratch(0, 0);
  ID3D11Texture2D* copy_texture = 0;

  U32 texture_height  = 0;
  U32 texture_width   = 0;
  DXGI_FORMAT format  = DXGI_FORMAT_UNKNOWN;
  U32 bytes_per_pixel = 0;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    texture.texture->GetDesc(&desc);
    desc.BindFlags      = 0;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    texture_height = desc.Height;
    texture_width  = desc.Width;
    format         = desc.Format;
    // note: For now only this one
    HandleLater(format == DXGI_FORMAT_R8G8B8A8_UNORM);
    bytes_per_pixel = 4;

    Data_buffer buffer = data_buffer_make(scratch.arena, texture_width * texture_height * bytes_per_pixel);
    
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem          = buffer.data;
    data.SysMemPitch      = texture_width * bytes_per_pixel;
    data.SysMemSlicePitch = Null; // note: Not used for 2d textures, only used for 3d textues

    hr = d3d->device->CreateTexture2D(&desc, &data, &copy_texture);
    HandleLater(hr == S_OK);

    d3d->context->CopyResource(copy_texture, texture.texture);
  }

  Image image = {};
  {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = d3d->context->Map(copy_texture, 0, D3D11_MAP_READ, 0, &mapped);
    {
      U64 size_for_image = texture_height * texture_width * bytes_per_pixel;
      image.bytes_per_pixel = (U64)bytes_per_pixel;
      image.width_in_px     = (U64)texture_width;
      image.height_in_px    = (U64)texture_height;
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
    d3d->context->Unmap(copy_texture, 0);
  }

  copy_texture->Release();
  end_scratch(&scratch);

  return image;
}

void r_export_texture(R_Target texture, Str8 file_name)
{
  if (!__r_is_target_valid_target(texture)) { BP; return; }

  Scratch scratch = get_scratch(0, 0);
  Image image = r_image_from_texture(scratch.arena, texture);

  Str8 file_path_nt = str8_copy_alloc(scratch.arena, file_name);
  int succ = stbi_write_png((char*)file_path_nt.data, (int)image.width_in_px, (int)image.height_in_px, (int)image.bytes_per_pixel, image.data, (int)(image.width_in_px * image.bytes_per_pixel));
  Handle(succ);
  end_scratch(&scratch);
}

void r_export_image(Image image, Str8 file_name)
{
  Scratch scratch   = get_scratch(0, 0);
  Str8 file_path_nt = str8_copy_alloc(scratch.arena, file_name);
  int succ          = stbi_write_png((char*)file_path_nt.data, (int)image.width_in_px, (int)image.height_in_px, (int)image.bytes_per_pixel, image.data, (int)(image.width_in_px * image.bytes_per_pixel));
  Handle(succ);
  end_scratch(&scratch);
}

R_Target r_load_texture_from_file(Str8 file_name)
{
  Scratch scratch = get_scratch(0, 0);
  D3D_State* d3d = r_get_state();

  Str8 file_name_nt = str8_copy_alloc(scratch.arena, file_name);

  int width = 0;
  int height = 0;
  int n_channels = 0;
  U8* image_bytes = stbi_load((char*)file_name_nt.data, &width, &height, &n_channels, 4);

  R_Target result_texture = {};
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

R_Target r_load_texture_from_image(Image image)
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

  ID3D11Texture2D* d3d_texture = 0;
  d3d->device->CreateTexture2D(&desc, &data, &d3d_texture);

  ID3D11RenderTargetView* d3d_texture_rtv = 0;
  d3d->device->CreateRenderTargetView(d3d_texture, 0, &d3d_texture_rtv);

  // On fail d3d will keep those pointer at 0
  R_Target texture = {};
  texture.texture     = d3d_texture;
  texture.texture_rtv = d3d_texture_rtv;
  return texture;
}

// note: I am not sure about this function, it has a bunch of restriction that i have to work relative to, 
//       so for now i will just blug in the bool for the caller to know if this was succ or fail
void r_copy_into_texture_from_texture(
  R_Target dest_texture,
  R_Target src_texture,
  B32* out_opt_is_succ
) {
  if (out_opt_is_succ) { *out_opt_is_succ = false; }
  if (!__r_is_target_valid_target(dest_texture))                                     { BP; if (out_opt_is_succ) { *out_opt_is_succ = false; } return; }
  if (!__r_is_target_valid_target(src_texture))                                      { BP; if (out_opt_is_succ) { *out_opt_is_succ = false; } return; }
  if (r_target_match(dest_texture, src_texture))                                     { BP; if (out_opt_is_succ) { *out_opt_is_succ = false; } return; }
  if (!v2f32_match(r_get_target_dims(dest_texture), r_get_target_dims(src_texture))) { BP; if (out_opt_is_succ) { *out_opt_is_succ = false; } return; }
  // note: Not checking if the texture are of the same pixel type, since we only support one right now

  D3D_State* d3d = r_get_state();
  d3d->context->CopyResource(dest_texture.texture, src_texture.texture);

  // todo: D3D doesnt return succ of fail for CopyResource, my check up top are not sufficient,
  //       so technically this func has a bug
}

V2F32 r_get_target_dims(R_Target target)
{
  if (!__r_is_target_valid_target(target)) { BP; return V2F32{}; }

  D3D11_TEXTURE2D_DESC desc = {};
  target.texture->GetDesc(&desc);
  return v2f32((F32)desc.Width, (F32)desc.Height);
}

///////////////////////////////////////////////////////////
// - Boring stuff with handles 
//
R_Target r_target_zero_handle()
{
  R_Target handle = {};
  return handle;
}

B32 r_target_match(R_Target target, R_Target other)
{
  B32 match = (
       target.texture     == other.texture
    && target.texture_rtv == other.texture_rtv     
  );
  return match;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// - Private stuff that is not for that caller to use or care about
//

///////////////////////////////////////////////////////////
// - Extra handle checks
//
B32 __r_is_target_valid_target(R_Target target)
{
  D3D_State* d3d = r_get_state();
  B32 valid = (
       target.texture     != 0
    && target.texture_rtv != 0
  );
  return valid;
}

B32 __r_is_target_valid_target_chain(R_Target target)
{
  B32 valid = (
       target.texture     != 0
    && target.texture_rtv != 0
    && target.swap_chain  != 0
    && target.__win32_window_handle_for_assert != 0
    // Comp device might be zero 
  );
  return valid;
}

#undef HR

#endif