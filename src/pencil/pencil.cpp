#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "platform.h"

struct G_state {
  B32 is_draw_mode;
};

void draw_rect(Grafics* grafics, F32 x, F32 y, F32 width, F32 height, F32 color)
{
  ID3D11DeviceContext* context = grafics->d3d_context;

  context->ClearState();
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  
  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    Grafics_rect_program_uniform_data u_data = {};
    u_data.rect_origin_x = x;
    u_data.rect_origin_y = y;
    u_data.rect_width    = width;
    u_data.rect_height   = height;
    u_data.window_width  = (F32)os_get_client_area_dims().x;
    u_data.window_height = (F32)os_get_client_area_dims().y;
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(u_data);
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags      = {};
    desc.StructureByteStride = {};
    grafics->d3d_device->CreateBuffer(&desc, NULL, &uniform_buffer);
    
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map((ID3D11Resource*)uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &u_data, sizeof(u_data));
    context->Unmap((ID3D11Resource*)uniform_buffer, 0);
  }

  context->VSSetShader(grafics->rect_program.v_shader, 0, 0);
  context->VSSetConstantBuffers(0, 1, &uniform_buffer);
  // uniform_buffer->Release();

  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    grafics->d3d_device->CreateRasterizerState(&desc, &rasterizer_state);
  }

  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  vp.Width    = (F32)os_get_client_area_dims().x;
  vp.Height   = (F32)os_get_client_area_dims().y;
  vp.MinDepth = 0;
  vp.MaxDepth = 1;
  context->RSSetState(rasterizer_state);
  // rasterizer_state->Release();
  context->RSSetViewports(1, &vp);
  context->PSSetShader(grafics->rect_program.p_shader, 0, 0);

  // todo: This shoud be a separate call to the draw thing that picks the render target
  context->OMSetRenderTargets(1, &grafics->draw_texture_rtv, 0);

  context->Draw(4, 0);
}

void pencil_update(OS_Event_list* events, Grafics* grafics)
{
  static B32 is_left_mouse_down = false;

  OS_Event* left_mouse_down_ev = 0;
  for (OS_Event* ev = events->first; ev != 0; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__Mouse_went_down)
    {
      Assert(is_left_mouse_down == 0);
      is_left_mouse_down = true;
      left_mouse_down_ev = ev;
      DllPop(events, ev);
      break;
    }
    else if (ev->kind == OS_Event_kind__Mouse_went_up)
    {
      Assert(is_left_mouse_down);
      is_left_mouse_down = false;
      left_mouse_down_ev = ev;
      DllPop(events, ev);
      break;
    }
  }

  // Drawing rects 
  if (is_left_mouse_down)
  {
    F32 rect_width = 10;
    F32 rect_height = 10;

    V2F32 new_pos = os_get_mouse_pos();
    V2F32 prev_pos = os_get_prev_mouse_pos();

    F32 dx = new_pos.x - prev_pos.x;
    F32 dy = new_pos.y - prev_pos.y;
    F32 length = sqrtf(dx * dx + dy * dy);
    U64 steps = (U64)length;
    for (U64 i = 0; i <= steps; i++) 
    {
      F32 t = (steps == 0) ? 0.0f : (F32)i / steps;
      F32 x = prev_pos.x + dx * t;
      F32 y = prev_pos.y + dy * t;
      draw_rect(grafics, x, y, rect_width, rect_height, 0);
    }
  }

  // Drawing the texture into the frame buffer
  {
    // todo: Draw a point into a texture and then draw that texture into the frame buffer
    ID3D11DeviceContext* context = grafics->d3d_context;
    context->ClearState();

    F32 color[4] = { 0, 0, 0, 0 };
    context->ClearRenderTargetView(grafics->frame_buffer_rtv, color);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(grafics->texture_to_screen_program.v_shader, 0, 0);

    V2S32 dims = os_get_client_area_dims();
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = (F32)dims.x;
    vp.Height   = (F32)dims.y;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    context->RSSetViewports(1, &vp);
    
    // todo: See if this sampler need to be removed later
    ID3D11SamplerState* sampler;
    {
      D3D11_SAMPLER_DESC desc = {};
      desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.MipLODBias = 0;
      desc.MaxAnisotropy = 1;
      desc.MinLOD = 0;
      desc.MaxLOD = D3D11_FLOAT32_MAX;
      grafics->d3d_device->CreateSamplerState(&desc, &sampler);
    }

    ID3D11Resource* resource = 0;
    grafics->draw_texture_rtv->GetResource(&resource);
    ID3D11ShaderResourceView* view = 0;
    grafics->d3d_device->CreateShaderResourceView(resource, NULL, &view);
    context->PSSetShader(grafics->texture_to_screen_program.p_shader, 0, 0);
    context->PSSetSamplers(0, 1, &sampler);
    context->PSSetShaderResources(0, 1, &view);
    resource->Release();

    context->OMSetRenderTargets(1, &grafics->frame_buffer_rtv, 0);

    context->Draw(4, 0);
  }
}

#endif