#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "pencil.h"

// todo: I dont like that P stored the shaders and all and we ahve to pss it in here like we do
void draw_rect(
  const Pencil_state* P,
  D3D_state* d3d, ID3D11RenderTargetView* render_target,  
  F32 x, F32 y, F32 width, F32 height, V4U8 color
) {
  ID3D11DeviceContext* context = d3d->context;
  ID3D11Device* device = d3d->device;

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
    u_data.red   = color.r;
    u_data.green = color.P;
    u_data.blue  = color.b;
    u_data.alpha = color.a;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(u_data);
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags      = {};
    desc.StructureByteStride = {};
    device->CreateBuffer(&desc, NULL, &uniform_buffer);
    
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map((ID3D11Resource*)uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &u_data, sizeof(u_data));
    context->Unmap((ID3D11Resource*)uniform_buffer, 0);
  }

  context->VSSetShader(P->rect_program.v_shader, 0, 0);
  context->VSSetConstantBuffers(0, 1, &uniform_buffer);
  // uniform_buffer->Release();

  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    device->CreateRasterizerState(&desc, &rasterizer_state);
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
  context->PSSetShader(P->rect_program.p_shader, 0, 0);

  // todo: This shoud be a separate call to the draw thing that picks the render target
  context->OMSetRenderTargets(1, &render_target, 0);

  context->Draw(4, 0);
}

Draw_record* get_new_draw_record_from_pool__nullable(Pencil_state* P)
{
  Draw_record* result = 0;
  if (P->first_free_draw_record)
  {
    result = P->first_free_draw_record;
    DllPopFront_Name(P, first_free_draw_record, last_free_draw_record, next, prev);
    *result = Draw_record{};
  }
  else if (P->count_of_pool_draw_records_in_use < DRAW_RECORDS_MAX_COUNT)
  {
    result = P->pool_of_draw_records + P->count_of_pool_draw_records_in_use;
    P->count_of_pool_draw_records_in_use += 1;
    *result = Draw_record{};
  }
  return result;
}

// todo: Does this really need to care about is_ui_capturing_mouse, or shoud this
//       be handled by the caller ????
Draw_record_registration_result register_new_draw_record(Pencil_state* P, D3D_state* d3d, B32 is_ui_capturing_mouse)
{
  if (P->is_mid_drawing || is_ui_capturing_mouse) { return Draw_record_registration_result{}; }

  // Freeing all the records that are in front of the current one
  if (P->current_record != 0)
  {
    for (Draw_record* record = P->last_record; record != 0;) 
    {
      if (record == P->current_record) { break; }
      record->texture_before_we_affected->Release();
      record->texture_after_we_affected->Release();
      
      DllPopBack_Name(P, first_record, last_record, next, prev);
      Draw_record* prev_record = record->prev;
      
      *record = Draw_record{};
      DllPushBack_Name(P, record, first_free_draw_record, last_free_draw_record, next, prev);
      
      record = prev_record;
    }
  }

  Draw_record* new_draw_record = get_new_draw_record_from_pool__nullable(P);
  if (new_draw_record == 0)
  {
    Draw_record* oldest_record = P->first_record;
    DllPopFront_Name(P, first_record, last_record, next, prev);
    
    *oldest_record = Draw_record{};
    DllPushBack_Name(P, oldest_record, first_free_draw_record, last_free_draw_record, next, prev);

    new_draw_record = get_new_draw_record_from_pool__nullable(P);
  }
  Assert(new_draw_record != 0); // This has to be true, its an ivariant

  // Adding the new draw record to the draw record queue
  DllPushBack_Name(P, new_draw_record, first_record, last_record, next, prev);  Assert(P->last_record == new_draw_record);

  new_draw_record->texture_before_we_affected = d3d_make_rtv(d3d, P->draw_texures_width, P->draw_texures_height);
  HandleLater(new_draw_record->texture_before_we_affected != 0);

  new_draw_record->texture_after_we_affected = d3d_make_rtv(d3d, P->draw_texures_width, P->draw_texures_height);
  HandleLater(new_draw_record->texture_after_we_affected != 0);

  P->current_record = new_draw_record;

  Draw_record_registration_result result = {};
  result.succ = true;
  result.record = new_draw_record;

  return result;
}

void copy_from_texture_to_texture(
  D3D_state* d3d,
  ID3D11RenderTargetView* dest_rtv, 
  ID3D11RenderTargetView* src_rtv
) {
  ID3D11Resource* dest_resource = 0;
  dest_rtv->GetResource(&dest_resource);

  ID3D11Resource* src_resource = 0;
  src_rtv->GetResource(&src_resource);
  
  d3d->context->CopyResource(dest_resource, src_resource);
}
 
void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse, D3D_state* d3d)
{
  // Handling signals
  /*
  {
    if (P->signal_new_pen_size)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->pen_size = P->new_pen_size;
        P->signal_new_pen_size = false;
        P->new_pen_size = 0;
      }
    }
    else
    if (P->signal_new_eraser_size)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->eraser_size = P->new_eraser_size;
        P->signal_new_eraser_size = false;
        P->new_eraser_size = 0;
      }
    }
    else 
    if (P->signal_swap_to_eraser)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->signal_swap_to_eraser = false;
        P->is_erasing_mode = true;
      }
    }
    else 
    if (P->signal_swap_to_pen)
    {
      Assert(P->is_mid_drawing == false); // Just making sure
      if (!P->is_mid_drawing)
      {
        P->signal_swap_to_pen = false;
        P->is_erasing_mode = false;
      }
    }
  }
  */
  
  B32 dont_start_drawing_this_frame = false;

  // These are different modes that we might want to change
  // if (!P->is_mid_drawing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)) 
  /*
  if (!P->is_mid_drawing && is_combination_to_switch_to_draw_mode)
  {
    dont_start_drawing_this_frame = true; 

    // todo: Look into this case, might be not implemented well 
    // This is here to be able to deal with current_record beeing 0
    Draw_record* next_record = 0;
    if (P->current_record == 0) {
      next_record = P->first_record;
    } 
    else if (P->current_record->next != 0) {
      next_record = P->current_record->next;
    }

    if (next_record)
    {
      RenderTexture future_texture = next_record->texture_after_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(
        P->draw_texture_always_fresh.texture, v2u64(0, 0), 
        future_texture.texture, v2u64(0, 0), 
        (U64)future_texture.texture.width, (U64)future_texture.texture.height
      );

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(
        P->draw_texture_not_that_fresh.texture, v2u64(0, 0), 
        future_texture.texture, v2u64(0, 0), 
        (U64)future_texture.texture.width, (U64)future_texture.texture.height
      );

      P->current_record = next_record;
    }
  }
  else // User wants to remove the last line they drew
  if (!P->is_mid_drawing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) 
  {
    dont_start_drawing_this_frame = true;

    if (P->current_record != 0)
    {
      Draw_record* record                    = P->current_record;
      RenderTexture texture_before_we_affected = record->texture_before_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(
        P->draw_texture_always_fresh.texture, v2u64(0, 0), 
        texture_before_we_affected.texture, v2u64(0, 0), 
        (U64)texture_before_we_affected.texture.width, (U64)texture_before_we_affected.texture.height
      );

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(
        P->draw_texture_not_that_fresh.texture, v2u64(0, 0), 
        texture_before_we_affected.texture, v2u64(0, 0), 
        (U64)texture_before_we_affected.texture.width, (U64)texture_before_we_affected.texture.height
      );
      
      P->current_record = P->current_record->prev;
    }
  }
  else // User want to clear the screen
  if (!P->is_mid_drawing && IsKeyPressed(KEY_DELETE))
  {
    dont_start_drawing_this_frame = true;

    // Creating a new current record
    Draw_record_registration_result record_reg = register_new_draw_record(P, is_ui_capturing_mouse);
    if (record_reg.succ)
    {
      Draw_record* record = record_reg.record;
      P->current_record = record_reg.record;
    
      U64 w = P->draw_texture_always_fresh.texture.width;
      U64 h = P->draw_texture_always_fresh.texture.height;
      
      // todo: These storing of 4 textures might be seprated into their own call
  
      // Clearing the texture
      fill_texture_with_color(P->draw_texture_always_fresh, { 0, 0, 0, 0 }, true);
  
      // Storing the prev texture state
      copy_from_texture_to_texture(
        P->current_record->texture_before_we_affected.texture, v2u64(0, 0),
        P->draw_texture_not_that_fresh.texture, v2u64(0, 0),
        P->draw_texures_width, P->draw_texures_height
      );
  
      // Storing the new texture state
      copy_from_texture_to_texture(
        P->current_record->texture_after_we_affected.texture, v2u64(0, 0),
        P->draw_texture_always_fresh.texture, v2u64(0, 0),
        P->draw_texures_width, P->draw_texures_height
      );
  
      // Matching the prev texture state to the new one
      copy_from_texture_to_texture(
        P->draw_texture_not_that_fresh.texture, v2u64(0, 0),
        P->draw_texture_always_fresh.texture, v2u64(0, 0),
        P->draw_texures_width, P->draw_texures_height
      );
    }

  }
  else // User wants to start using the eraser pen 
  if (!P->is_mid_drawing && IsKeyPressed(KEY_E))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = true;
  }
  else // User wants to start using the brush/pen
  if (!P->is_mid_drawing && IsKeyPressed(KEY_B))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = false;
  } 
  else
  if (IsKeyPressed(KEY_TAB))
  {
    ToggleBool(P->show_brush_ui_menu);
    ui_reset_active_match(P->brush_menu_ui_id);
  }
  */

  if (dont_start_drawing_this_frame) { goto __active_draw_update_routine_end__; }

  if (!P->is_mid_drawing && !is_ui_capturing_mouse && os_mouse_button_down(Mouse_button__Left)) 
  {
    Draw_record_registration_result record_registation = register_new_draw_record(P, d3d, is_ui_capturing_mouse);
    if (record_registation.succ)
    {
      // todo: What is .succ here about, do we need it ???
      P->is_mid_drawing = true;
    }
  }
  else // Updating active drawing 
  if (P->is_mid_drawing && os_mouse_button_down(Mouse_button__Left))
  {
    V2F32 new_pos = os_get_mouse_pos();
    // V2F32 prev_pos = os_get_prev_mouse_pos();
    
    // todo: This is not the best way to have this here like this, but for now thats how it is
    // V4U8 color = { 0, 0, 0 ,0 };
    // if (!P->is_erasing_mode) { color = P->pen_color; }
    // U64 pen_size = P->eraser_size;
    // if (!P->is_erasing_mode) { pen_size = P->pen_size; }
    draw_rect(P, d3d, P->draw_texture_always_fresh, new_pos.x, new_pos.y, 10, 10, yellow());

    // todo: Update this comment here (We no longer use cpu side image)
    // note: Here is the only time when the cpu side image is not synced to the gpu one. 
    //       But we do later sync it when we stop drawing.
  }
  else // Here we finalise the draw recordd that the user have been drawing
  if (P->is_mid_drawing && os_mouse_button_went_up(Mouse_button__Left))
  {
    Assert(P->current_record != 0);
    Assert(P->current_record->texture_after_we_affected != 0);  // These are expected to already be allocated by this point
    Assert(P->current_record->texture_before_we_affected != 0); // These are expected to already be allocated by this point
    
    P->is_mid_drawing = false;

    U64 draw_t_width = P->draw_texures_width;
    U64 draw_t_height = P->draw_texures_height;

    Draw_record* record = P->current_record;
    ID3D11RenderTargetView* texture_before_we_affected = record->texture_before_we_affected;
    ID3D11RenderTargetView* texture_after_we_affected  = record->texture_after_we_affected;
    
    // note: By this point, the fresh draw texture is the new final version of what the user has draw

    // Storing the prev version of the draw texture 
    copy_from_texture_to_texture(d3d, texture_before_we_affected, P->draw_texture_not_that_fresh);

    // Storing the new version of the draw texture
    copy_from_texture_to_texture(d3d, texture_after_we_affected, P->draw_texture_always_fresh);

    // Updating the prev version of the draw texture to match the new version
    copy_from_texture_to_texture(d3d, P->draw_texture_not_that_fresh, P->draw_texture_always_fresh);
  }

  __active_draw_update_routine_end__: {};
}

void pencil_render(const Pencil_state* P, D3D_state* d3d)
{
  // Drawing the texture into the frame buffer
  {
    ID3D11DeviceContext* context = d3d->context;
    ID3D11Device*        device  = d3d->device;
    
    context->ClearState();

    ID3D11RenderTargetView* frame_buffer_rtv = d3d_get_frame_buffer_rtv(d3d);

    F32 color[4] = { 0, 0, 0, 0 };
    context->ClearRenderTargetView(frame_buffer_rtv, color);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(P->texture_to_screen_program.v_shader, 0, 0);

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
      device->CreateSamplerState(&desc, &sampler);
    }

    ID3D11Resource* resource = 0;
    P->draw_texture_always_fresh->GetResource(&resource);
    ID3D11ShaderResourceView* view = 0;
    device->CreateShaderResourceView(resource, NULL, &view);
    context->PSSetShader(P->texture_to_screen_program.p_shader, 0, 0);
    context->PSSetSamplers(0, 1, &sampler);
    context->PSSetShaderResources(0, 1, &view);
    resource->Release();

    context->OMSetRenderTargets(1, &frame_buffer_rtv, 0);

    context->Draw(4, 0);

    frame_buffer_rtv->Release();
  }

}


#endif