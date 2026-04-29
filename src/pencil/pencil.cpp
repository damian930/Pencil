#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "platform.h"

// todo: I dont yet know where to have these, so just having them here

// D3D_program rect_program;
// D3D_program texture_to_screen_program;

// ID3D11RenderTargetView* draw_texture_rtv;
// ID3D11RenderTargetView* frame_buffer_rtv;

struct Draw_record {
  ID3D11RenderTargetView* texture_before_we_affected; // This is allocated when done drawing
  ID3D11RenderTargetView* texture_after_we_affected;  // This is allocated when done drawing

  // These are also used for draw record free list
  Draw_record* next;
  Draw_record* prev;
};

struct Pencil_state {
  Arena* arena;
  Arena* frame_arena;
  
  U64 pen_size;
  V4U8 pen_color;

  U64 eraser_size;

  U64 draw_texures_width;
  U64 draw_texures_height;
  ID3D11RenderTargetView* draw_texture_always_fresh; 
  ID3D11RenderTargetView* draw_texture_not_that_fresh;

  // Pool of draw records
  #define DRAW_RECORDS_MAX_COUNT 50
  Draw_record pool_of_draw_records[DRAW_RECORDS_MAX_COUNT];
  U64 count_of_pool_draw_records_in_use; // This inludes if they are in the free list
  Draw_record* first_free_draw_record;
  Draw_record* last_free_draw_record;
  
  Draw_record* first_record;
  Draw_record* last_record;
  Draw_record* current_record;

  // Stuff for while drawing
  B32 is_mid_drawing;
  B32 is_erasing_mode;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;
  //
  B32 signal_new_eraser_size;
  U64 new_eraser_size;
  //
  B32 signal_swap_to_eraser;
  B32 signal_swap_to_pen;

  // Misc
  // Font font_texture_for_ui;
  V2U64 last_screen_dims;
  B32 show_brush_ui_menu;
  Str8 brush_menu_ui_id;

  // Stuff for drawing that i dont yet have as a separate thing
  D3D_program rect_program;  
  D3D_program texture_to_screen_program;
};

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
    u_data.green = color.g;
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

void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse, const OS_Event* os_event, D3D_state* d3d)
{
  // Handling signals
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
    Draw_record_registration_result record_reg = register_new_draw_record(G, is_ui_capturing_mouse);
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

  // User is about to start drawing
  // todo: Catch when the mouse goes down
  B32 mouse_went_down_this_frame = false;
  if (os_event->kind == OS_Event_kind__)


  if (!P->is_mid_drawing && !is_ui_capturing_mouse && mouse_went_down_this_frame) 
  {
    Draw_record_registration_result record_registation = register_new_draw_record(G, is_ui_capturing_mouse);
    if (record_registation.succ)
    {
      P->is_mid_drawing = true;
    }
  }
  else // Updating active drawing 
  if (P->is_mid_drawing && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
  {
    Vector2 new_pos = GetMousePosition();
    Vector2 prev_pos = Vector2Subtract(new_pos, GetMouseDelta());
    
    // todo: This is not the best way to have this here like this, but for now thats how it is
    V4U8 color = { 0, 0, 0 ,0 };
    if (!P->is_erasing_mode) { color = P->pen_color; }
    U64 pen_size = P->eraser_size;
    if (!P->is_erasing_mode) { pen_size = P->pen_size; }
    draw_onto_texture(P->draw_texture_always_fresh, new_pos, prev_pos, pen_size, color, P->is_erasing_mode);

    // todo: Update this comment here (We no longer use cpu side image)
    // note: Here is the only time when the cpu side image is not synced to the gpu one. 
    //       But we do later sync it when we stop drawing.
  }
  else // Here we finalise the draw recordd that the user have been drawing
  if (P->is_mid_drawing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
  {
    Assert(P->current_record != 0);
    Assert(P->current_record->texture_after_we_affected.texture.id != 0);  // These are expected to already be allocated by this point
    Assert(P->current_record->texture_before_we_affected.texture.id != 0); // These are expected to already be allocated by this point
    
    P->is_mid_drawing = false;

    U64 draw_t_width = P->draw_texture_always_fresh.texture.width;
    U64 draw_t_height = P->draw_texture_always_fresh.texture.height;

    Draw_record* record = P->current_record;
    RenderTexture* texture_before_we_affected = &record->texture_before_we_affected;
    RenderTexture* texture_after_we_affected  = &record->texture_after_we_affected;
    
    // note: By this point, the fresh draw texture is the new final version of what the user has draw

    // Storing the prev version of the draw texture 
    copy_from_texture_to_texture(
      texture_before_we_affected->texture, v2u64(0, 0),
      P->draw_texture_not_that_fresh.texture, v2u64(0, 0),
      P->draw_texures_width, P->draw_texures_height
    );

    // Storing the new version of the draw texture
    copy_from_texture_to_texture(
      texture_after_we_affected->texture, v2u64(0, 0),
      P->draw_texture_always_fresh.texture, v2u64(0, 0),
      P->draw_texures_width, P->draw_texures_height
    );

    // Updating the prev version of the draw texture to match the new version
    copy_from_texture_to_texture(
      P->draw_texture_not_that_fresh.texture, v2u64(0, 0),
      P->draw_texture_always_fresh.texture, v2u64(0, 0),
      P->draw_texures_width, P->draw_texures_height
    );
  }

  __active_draw_update_routine_end__: {};
}

void __pencil_update_test__(Pencil_state* P, OS_Event_list* events, D3D_state* d3d)
{
  static B32 is_initialised = false;
  if (!is_initialised)
  {
    is_initialised = true;

    *P = Pencil_state{};
    { // draw_texture_rtv  
      
      D3D11_TEXTURE2D_DESC texture_desc = {};
      texture_desc.Width            = os_get_client_area_dims().x;
      texture_desc.Height           = os_get_client_area_dims().y;
      texture_desc.MipLevels        = 1;
      texture_desc.ArraySize        = 1;
      texture_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
      texture_desc.SampleDesc.Count = 1;
      texture_desc.Usage            = D3D11_USAGE_DEFAULT; // Read and write by the gpu
      texture_desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
      
      HRESULT hr = S_OK;
      ID3D11Texture2D* texture = 0;
      hr = d3d->device->CreateTexture2D(&texture_desc, 0, &texture);
      HandleLater(hr == S_OK);

      hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &P->draw_texture_rtv);
      HandleLater(hr == S_OK);
    }

    B32 rect_program_suc = true;
    P->rect_program = grafics_create_program_from_file(d3d->device, L"../data/rect_program_shader.hlsl", "vs_main", "ps_main", &rect_program_suc);
    Assert(rect_program_suc);

    B32 texture_to_screen_succ = true;
    P->texture_to_screen_program = grafics_create_program_from_file(d3d->device, L"../data/texture_to_screen_program_shader.hlsl", "vs_main", "ps_main", &texture_to_screen_succ);
    Assert(texture_to_screen_succ);
  }

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
      draw_rect(P, d3d, P->draw_texture_rtv, x, y, rect_width, rect_height, pink());
    }
  }

  // Drawing the texture into the frame buffer
  {
    // todo: Draw a point into a texture and then draw that texture into the frame buffer
    ID3D11DeviceContext* context = d3d->context;
    ID3D11Device* device = d3d->device;
    
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
    P->draw_texture_rtv->GetResource(&resource);
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