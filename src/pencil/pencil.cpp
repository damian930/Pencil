#ifndef PENCIL_CPP
#define PENCIL_CPP

#include "pencil.h"

// #include "ui/ui_core.h"
// #include "ui/ui_core.cpp"

// todo: I dont like that P stored the shaders and all and we ahve to pss it in here like we do

// todo: Have a draw circle separate call in the d3d layer
/*
void pencil_draw_circle(
  const Pencil_state* P, 
  D3D_State* d3d, ID3D11RenderTargetView* render_target,
  F32 x_origin, F32 y_origin, F32 r, V4F32 color
) {
  ID3D11DeviceContext* context = d3d->context;
  ID3D11Device* device = d3d->device;

  context->ClearState();
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  
  // Setting the uniform buffer
  ID3D11Buffer* uniform_buffer = 0;
  {
    D3D_Circle_program_data u_data = {};
    u_data.origin_x      = x_origin; 
    u_data.origin_y      = y_origin; 
    u_data.radius        = r;
    u_data.window_width  = (F32)os_get_client_area_dims().x;
    u_data.window_height = (F32)os_get_client_area_dims().y;
    u_data.color         = color; 

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

  context->VSSetShader(P->circle_program.v_shader, 0, 0);
  context->VSSetConstantBuffers(0, 1, &uniform_buffer);

  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    device->CreateRasterizerState(&desc, &rasterizer_state);
  }

  // todo: Dont do this any more, this is dont in the begin call for rendering
  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  vp.Width    = (F32)os_get_client_area_dims().x;
  vp.Height   = (F32)os_get_client_area_dims().y;
  vp.MinDepth = 0;
  vp.MaxDepth = 1;
  context->RSSetState(rasterizer_state);
  context->RSSetViewports(1, &vp);
  
  context->PSSetShader(P->circle_program.p_shader, 0, 0);
  context->PSSetConstantBuffers(0, 1, &uniform_buffer);
  
  // todo: This shoud be a separate call to the draw thing that picks the render target
  context->OMSetRenderTargets(1, &render_target, 0);

  context->Draw(4, 0);

  rasterizer_state->Release();
  uniform_buffer->Release();
}
*/

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
Draw_record_registration_result register_new_draw_record(Pencil_state* P, D3D_State* d3d, B32 is_ui_capturing_mouse)
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
  D3D_State* d3d,
  ID3D11RenderTargetView* dest_rtv, 
  ID3D11RenderTargetView* src_rtv
) {
  ID3D11Resource* dest_resource = 0;
  dest_rtv->GetResource(&dest_resource);

  ID3D11Resource* src_resource = 0;
  src_rtv->GetResource(&src_resource);
  
  d3d->context->CopyResource(dest_resource, src_resource);
}
 
void pencil_update(Pencil_state* P, B32 is_ui_capturing_mouse, D3D_State* d3d)
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
  if (!P->is_mid_drawing && os_key_down(Key__Control) && os_key_down(Key__Shift) && os_key_went_down(Key__Z)) 
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
      ID3D11RenderTargetView* future_texture = next_record->texture_after_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(d3d, P->draw_texture_always_fresh, future_texture);

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(d3d, P->draw_texture_not_that_fresh, future_texture);

      P->current_record = next_record;
    }
  }
  else // User wants to remove the last line they drew
  if (!P->is_mid_drawing && os_key_down(Key__Control) && os_key_went_down(Key__Z)) 
  {
    dont_start_drawing_this_frame = true;

    if (P->current_record != 0)
    {
      Draw_record* record = P->current_record;
      ID3D11RenderTargetView* texture_before_we_affected = record->texture_before_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(d3d, P->draw_texture_always_fresh, texture_before_we_affected);

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(d3d, P->draw_texture_not_that_fresh, texture_before_we_affected);

      P->current_record = P->current_record->prev;
    }
  }
  else // User want to clear the screen
  if (!P->is_mid_drawing && os_key_went_up(Key__Delete))
  {
    dont_start_drawing_this_frame = true;

    // Creating a new current record
    Draw_record_registration_result record_reg = register_new_draw_record(P, d3d, is_ui_capturing_mouse);
    if (record_reg.succ)
    {
      Draw_record* record = record_reg.record;
      P->current_record = record_reg.record;
    
      U64 w = P->draw_texures_width;
      U64 h = P->draw_texures_height;
      
      // Clearing the texture
      F32 _color_[4] = { 0, 0, 0, 0 };
      d3d->context->ClearRenderTargetView(P->draw_texture_always_fresh, _color_);
  
      // Storing the prev texture state
      copy_from_texture_to_texture(d3d, P->current_record->texture_before_we_affected, P->draw_texture_not_that_fresh);
  
      // Storing the new texture state
      copy_from_texture_to_texture(d3d, P->current_record->texture_after_we_affected, P->draw_texture_always_fresh);

      // Matching the prev texture state to the new one
      copy_from_texture_to_texture(d3d, P->draw_texture_not_that_fresh, P->draw_texture_always_fresh);
    }
  }
  else // User wants to start using the eraser pen 
  if (!P->is_mid_drawing && os_key_went_up(Key__E))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = true;
  }
  else // User wants to start using the brush/pen
  if (!P->is_mid_drawing && os_key_went_up(Key__B))
  {
    dont_start_drawing_this_frame = true;
    P->is_erasing_mode = false;
  } 
  /*
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
    V2F32 prev_pos = os_get_prev_mouse_pos();
    
    // Drawing a continuos line based on delta
    {
      V4F32 color  = P->pen_color;
      F32 pen_size = (F32)P->pen_size;
      if (P->is_erasing_mode) { 
        color    = v4f32(0.0f, 0.0f, 0.0f, 0.0f); 
        pen_size = (F32)P->eraser_size;
      }

      F32 dx = new_pos.x - prev_pos.x;
      F32 dy = new_pos.y - prev_pos.y;
      F32 length = sqrtf(dx * dx + dy * dy);
      U64 steps = (U64)length;
      for (U64 i = 0; i <= steps; i++) 
      {
        F32 t = (steps == 0) ? 0.0f : (F32)i / steps;
        F32 x = prev_pos.x + dx * t;
        F32 y = prev_pos.y + dy * t;
        d3d_draw_circle(d3d, P->draw_texture_always_fresh, x, y, pen_size, color);
      }
    }

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

/*
void pencil_build_ui(Pencil_state* P, RLI_Event_list* rli_events)
{
  int w = os_get_client_area_dims().x;
  int h = os_get_client_area_dims().y;
  ui_begin_build((F32)GetScreenWidth(), (F32)GetScreenHeight(), (F32)GetMouseX(), (F32)GetMouseY());

  // ui_push_font(G->font_texture_for_ui);

  ui_set_next_border({ 2, { 255, 0, 0, 255 } });
  ui_set_next_size_x(ui_p_of_p(1, 1));
  ui_set_next_size_y(ui_p_of_p(1, 1));
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* top_wrapper = ui_box_make(Str8{}, 0);

  UI_Box* content_inner = 0;
  UI_Parent(top_wrapper)
  {
    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_color({ 255, 0, 0, 255 });
    UI_Box* left_border = ui_box_make(Str8{}, 0);
    
    ui_set_next_size_x(ui_p_of_p(1, 0));
    ui_set_next_size_y(ui_p_of_p(1, 0));
    ui_set_next_layout_axis(Axis2__y);
    UI_Box* content_outer = ui_box_make(Str8{}, 0);

    UI_Parent(content_outer)
    {
      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_color({ 255, 0, 0, 255 });
      UI_Box* top_border = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 0));
      ui_set_next_size_y(ui_p_of_p(1, 0));
      ui_set_next_layout_axis(Axis2__y);
      content_inner = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_color({ 255, 0, 0, 255 });
      UI_Box* bottom_border = ui_box_make(Str8{}, 0);
    }

    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_color({ 255, 0, 0, 255 });
    UI_Box* right_border = ui_box_make(Str8{}, 0);
  }

  if (G->show_brush_ui_menu)
  {
    UI_Parent(content_inner)
    {
      ui_set_next_flags(UI_Box_flag__floating);
      ui_set_next_size_x(ui_p_of_p(1, 1)); ui_set_next_size_y(ui_p_of_p(1, 1));
      ui_set_next_layout_axis(Axis2__y);
      UI_Box* brush_wrapper_box_x = ui_box_make(Str8FromC("Wrapper of floating boxes id"), 0);
      
      Str8 brush_floating_hook_box_id = Str8FromC("Brush floating box hook id");
      G->brush_menu_ui_id = brush_floating_hook_box_id;
  
      static F32 drag_mouse_hold_pos_x = 0;
      static F32 drag_mouse_hold_pos_y = 0;
  
      static F32 x_offset = 50.0f;
      static F32 y_offset = 100.0f;
  
      static B32 is_brush_menu_open = false;
  
      F32 x_offset_old = x_offset;
      F32 y_offset_old = y_offset;
  
      UI_Actions brush_box_actions = ui_actions_from_id(brush_floating_hook_box_id, rli_events);
      UI_Box_data brush_box_actions_data = ui_get_box_data_prev_frame(brush_floating_hook_box_id);
      if (!brush_box_actions.was_down && brush_box_actions.is_down)
      {
        ui_set_active(brush_floating_hook_box_id);
        drag_mouse_hold_pos_x = (F32)GetMouseX() - brush_box_actions_data.on_screen_rect.x;
        drag_mouse_hold_pos_y = (F32)GetMouseY() - brush_box_actions_data.on_screen_rect.y;
      }
      else if (brush_box_actions.is_down)
      {
        UI_Box_data float_space_box_data = ui_get_box_data_prev_frame(brush_wrapper_box_x->id);
        x_offset = (F32)GetMouseX() - drag_mouse_hold_pos_x - float_space_box_data.on_screen_rect.x;
        y_offset = (F32)GetMouseY() - drag_mouse_hold_pos_y - float_space_box_data.on_screen_rect.y;
      }
      else
      {
        ui_reset_active_match(brush_floating_hook_box_id);
        drag_mouse_hold_pos_x = 0;
        drag_mouse_hold_pos_y = 0;
      }
  
      UI_Parent(brush_wrapper_box_x)
      {
        ui_spacer(ui_px(y_offset_old));
  
        ui_set_next_size_x(ui_p_of_p(1, 1)); ui_set_next_size_y(ui_p_of_p(1, 1));
        ui_set_next_layout_axis(Axis2__x);
        UI_Box* brush_wrapper_box_y = ui_box_make({}, 0);
        UI_Parent(brush_wrapper_box_y)
        {
          ui_spacer(ui_px(x_offset_old));
  
          ui_set_next_size_x(ui_px(200)); ui_set_next_size_y(ui_children_sum());
          ui_set_next_layout_axis(Axis2__y);
          // ui_set_next_corner_radius(0.15f);
          ui_set_next_color({ 255, 255, 255, 255 });
          UI_Box* brushes_menu_box = ui_box_make({}, 0);
          UI_Parent(brushes_menu_box)
          {
            // Header hook box
            ui_set_next_size_x(ui_p_of_p(1, 0)); ui_set_next_size_y(ui_children_sum());
            ui_set_next_layout_axis(Axis2__y);
            ui_set_next_color({ 151, 184, 210, 255 });
            UI_Box* hook_box = ui_box_make(brush_floating_hook_box_id, 0);
            UI_Parent(hook_box)
            {
              ui_spacer(ui_px(5));
  
              UI_PaddedBox(ui_px(5), Axis2__x)
              {
                ui_set_next_text_color({ 0, 0, 0, 255 });
                ui_set_next_font_size(24);
                ui_label_c("Brushes");
  
                ui_set_next_color({ 255, 0, 0, 255 });
                
              }
  
              ui_spacer(ui_px(5));
            }
    
            UI_XStack()
            {
              ui_spacer(ui_p_of_p(1, 0));
  
              UI_PaddedBox(ui_px(5), Axis2__y) 
              {
                ui_spacer(ui_px(10));
      
                if (G->is_erasing_mode)
                {
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style size_slider_style = {};
                    size_slider_style.height         = 20;
                    size_slider_style.width          = 100;
                    size_slider_style.hover_color    = { 169, 205, 246, 255 };
                    size_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    size_slider_style.text_color     = { 0, 0, 0, 255 };
                    size_slider_style.font_size      = 20;
                    size_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    size_slider_style.fmt_str = "%.0f";
        
                    U64 new_eraser_size = (U64)ui_slider(Str8FromC("Eraser size slider id"), &size_slider_style, (F32)G->eraser_size, 1, 100, rli_events);
                    if (new_eraser_size != G->eraser_size)
                    {
                      G->signal_new_eraser_size = true;
                      G->new_eraser_size = new_eraser_size;
                    } 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Size");
                  }
        
                  ui_spacer(ui_px(10));
    
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    ui_set_next_color({ 175, 203, 255, 255 });
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    UI_PaddedBox(ui_px(5), Axis2__x)
                    {
                      UI_Actions pen_button = ui_button(Str8FromC("Brush - [B]## button"), rli_events);
                      if (pen_button.is_clicked)
                      {
                        G->signal_swap_to_pen= true;
                      }
                    }
                  }
                }
                else
                {
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style size_slider_style = {};
                    size_slider_style.height         = 20;
                    size_slider_style.width          = 100;
                    size_slider_style.hover_color    = { 169, 205, 246, 255 };
                    size_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    size_slider_style.text_color     = { 0, 0, 0, 255 };
                    size_slider_style.font_size      = 20;
                    size_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    size_slider_style.fmt_str = "%.0f";
        
                    U64 new_pen_size = (U64)ui_slider(Str8FromC("Pen size slider id"), &size_slider_style, (F32)G->pen_size, 1, 100, rli_events);
                    clamp_u64_inplace(&new_pen_size, 1, 100);
                    if (new_pen_size != G->pen_size)
                    {
                      G->signal_new_pen_size = true;
                      G->new_pen_size = new_pen_size;
                    } 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Size");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style red_slider_style = {};
                    red_slider_style.height         = 20;
                    red_slider_style.width          = 100;
                    red_slider_style.hover_color    = { 169, 205, 246, 255 };
                    red_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    red_slider_style.text_color     = { 0, 0, 0, 255 };
                    red_slider_style.font_size      = 20;
                    red_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    red_slider_style.fmt_str = "%.0f";
        
                    U8 new_r_color = (U8)ui_slider(Str8FromC("Red slider style id"), &red_slider_style, (F32)G->pen_color.r, 0, 255, rli_events);
                    G->pen_color.r = new_r_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Red");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style green_slider_style = {};
                    green_slider_style.height         = 20;
                    green_slider_style.width          = 100;
                    green_slider_style.hover_color    = { 169, 205, 246, 255 };
                    green_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    green_slider_style.text_color     = { 0, 0, 0, 255 };
                    green_slider_style.font_size      = 20;
                    green_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    green_slider_style.fmt_str = "%.0f";
        
                    U8 new_g_color = (U8)ui_slider(Str8FromC("Green slider style id"), &green_slider_style, (F32)G->pen_color.g, 0, 255, rli_events);
                    G->pen_color.g = new_g_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Green");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style blue_slider_style = {};
                    blue_slider_style.height         = 20;
                    blue_slider_style.width          = 100;
                    blue_slider_style.hover_color    = { 169, 205, 246, 255 };
                    blue_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    blue_slider_style.text_color     = { 0, 0, 0, 255 };
                    blue_slider_style.font_size      = 20;
                    blue_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    blue_slider_style.fmt_str = "%.0f";
        
                    U8 new_b_color = (U8)ui_slider(Str8FromC("Blue slider style id"), &blue_slider_style, (F32)G->pen_color.b, 0, 255, rli_events);
                    G->pen_color.b = new_b_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Blue");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style alpa_slider_style = {};
                    alpa_slider_style.height         = 20;
                    alpa_slider_style.width          = 100;
                    alpa_slider_style.hover_color    = { 169, 205, 246, 255 };
                    alpa_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    alpa_slider_style.text_color     = { 0, 0, 0, 255 };
                    alpa_slider_style.font_size      = 20;
                    alpa_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    alpa_slider_style.fmt_str = "%.0f";
        
                    U8 new_a_color = (U8)ui_slider(Str8FromC("Alpha slider style id"), &alpa_slider_style, (F32)G->pen_color.a, 0, 255, rli_events);
                    G->pen_color.a = new_a_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Alpha");
                  }
                  
                  ui_spacer(ui_px(10));
          
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    ui_set_next_color({ 175, 203, 255, 255 });
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    UI_PaddedBox(ui_px(5), Axis2__x)
                    {
                      UI_Actions eraser_button = ui_button(Str8FromC("Eraser - [E]## button"), rli_events);
                      if (eraser_button.is_clicked)
                      {
                        G->signal_swap_to_eraser= true;
                      }
                    }
                  }
                }
              }
  
              ui_spacer(ui_p_of_p(1, 0));
            }
  
          }
        }
      }
    }
  }

  ui_end_build();
}
*/

void pencil_render(const Pencil_state* P, D3D_State* d3d)
{
  // Drawing the texture into the frame buffer
  {
    ID3D11DeviceContext* context = d3d->context;
    ID3D11Device*        device  = d3d->device;
    
    context->ClearState();

    ID3D11RenderTargetView* frame_buffer_rtv = d3d_get_frame_buffer_rtv(d3d);

    F32 color[4] = { 0, 0, 0, 0 };
    context->ClearRenderTargetView(frame_buffer_rtv, color);

    // Alpha blend state
    // ID3D11BlendState* blend_state = 0;
    // {
    //   // enable alpha blending
    //   D3D11_BLEND_DESC desc = {};
    //   desc.RenderTarget[0].BlendEnable           = TRUE;
    //   desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    //   desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    //   desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    //   desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
    //   desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    //   desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    //   desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    //   device->CreateBlendState(&desc, &blend_state);
    // }

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(P->texture_to_screen_program.v_shader, 0, 0);

    V2F32 dims = os_get_client_area_dims();
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = dims.x;
    vp.Height   = dims.y;
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

    // context->OMSetBlendState(blend_state, NULL, ~0U);
    context->OMSetRenderTargets(1, &frame_buffer_rtv, 0);

    context->Draw(4, 0);

    frame_buffer_rtv->Release();
  }

}

#if DEBUG_MODE
void __debug_export_current_record_images(const Pencil_state* P, D3D_State* d3d)
{
  // todo: I dont like the api for the D3D_Texture_result. 
  //       The way we have to get the texture out and it is 0 if the succ is false
  //       and then when we have to release it and then the pointer is the result is kind of 
  //       also the same the one we released. Sucks. I would like there to just be a singe
  //       poiner or whatever that i have to work with.
  //       The less things to manage and think about, the better.

  // Loading up always_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result fresh_texture_res = d3d_texture_from_rtv(P->draw_texture_always_fresh);
    if (fresh_texture_res.succ)
    {
      Image image = d3d_export_texture(P->frame_arena, d3d, fresh_texture_res.texture);
      stbi_flip_vertically_on_write(true); 
      int succ = stbi_write_png("always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      fresh_texture_res.texture->Release();
    } 
    else { Assert(0); }
  }

  // Loading up not_fresh_texture
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result not_fresh_texture_res = d3d_texture_from_rtv(P->draw_texture_not_that_fresh);
    if (not_fresh_texture_res.succ)
    {
      Image image = d3d_export_texture(P->frame_arena, d3d, not_fresh_texture_res.texture);
      int succ = stbi_write_png("not_always_fresh_texture.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      not_fresh_texture_res.texture->Release();
    } 
    else { Assert(0); }
  }
  
  // Loading up current texture_after_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result texture_res = d3d_texture_from_rtv(P->current_record->texture_after_we_affected);
    if (texture_res.succ)
    {
      Image image = d3d_export_texture(P->frame_arena, d3d, texture_res.texture);
      int succ = stbi_write_png("current_texture_after_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      texture_res.texture->Release();
    } 
    else { Assert(0); }
  }

  // Loading up current texture_before_we_affected
  if (P->current_record != 0)
  DeferInitReleaseLoop(Scratch scratch = get_scratch(0, 0), end_scratch(&scratch))
  {
    D3D_Texture_result texture_res = d3d_texture_from_rtv(P->current_record->texture_before_we_affected);
    if (texture_res.succ)
    {
      Image image = d3d_export_texture(P->frame_arena, d3d, texture_res.texture);
      int succ = stbi_write_png("current_texture_before_we_affected.png", (int)image.width_in_px, (int)image.height_in_px, 4, image.data, (int)image.width_in_px * (int)image.bytes_per_pixel);
      Assert(succ);
      texture_res.texture->Release();
    } 
    else { Assert(0); }
  }
}
#endif

#endif