#ifndef DRAW_CPP
#define DRAW_CPP

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"

///////////////////////////////////////////////////////////
// - State 
//
global D_State __d_g_state = {};

D_State* d_get_state() { return &__d_g_state; }

void d_init()
{
  __d_g_state.arena_for_draw_commands = arena_alloc(Megabytes(64));
}

void d_release() 
{ 
  arena_release(__d_g_state.arena_for_draw_commands);
  __d_g_state = D_State{};
}

///////////////////////////////////////////////////////////
// - Batching 
//
void d_begin_batching(R_Target target) 
{ 
  // todo: If targe t== 0 return

  D_State* draw_state = d_get_state();
  
  draw_state->command_batch_list = {};
  arena_clear(draw_state->arena_for_draw_commands); 

  draw_state->defaults.blend_kind    = D3D_Blend_kind__alpha;
  draw_state->defaults.render_target = target.texture_rtv; 
  draw_state->defaults.scissor_rect  = rect_make(0.0f, 0.0f, r_get_target_dims(target).x, r_get_target_dims(target).y);

  draw_state->current_blend_kind_count    = 0;
  draw_state->current_render_target_count = 0;
  draw_state->current_scissor_rect_count  = 0;
}

// todo: THis does not take it target and this makes it less clear about the idea for the call and what it does,
//       make it better 
void d_end_batching() { /*Nothing here*/ }

D_Command_batch_list* d_get_batch_list() { return &d_get_state()->command_batch_list; }

D_Command_batch* d_add_new_batch(D_Command_type command_type, ID3D11Texture2D* texture)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  // note:
  // If this static assert fails, that mean that the batch struct has changed. 
  // To resolve the assert, go see what field in the struct you have that you dont set here to 
  // the new batch, set that field. Then manually change the value you compare agains sizeof
  // to be able to assert the next time. 
  // Why is it important?
  // It is important to now have bugs, since to have batches working we have to change the code in couple 
  // of places after we add settings to the Batch struct. This place is 1 of them. 
  StaticAssert(sizeof(D_Command_batch) == 80);

  D_Command_batch* new_batch = ArenaPush(arena, D_Command_batch);
  new_batch->command_type = command_type;
  new_batch->texture      = texture;
  new_batch->rtv          = __d_get_current_render_target__defaults();
  new_batch->scissor_rect = __d_get_current_scissor_rect__default();
  new_batch->blend_kind   = __d_get_current_blend_kind__defaults();
  
  QueuePushBack_Name(&draw_state->command_batch_list, new_batch, first, last, next_batch);
  draw_state->command_batch_list.count += 1;

  return new_batch;
}

D_Command_batch* d_get_or_add_batch_for_settings(D_Command_type command_type, ID3D11Texture2D* texture)
{
  // note:
  // If this static assert fails, that mean that the batch struct has changed. 
  // To resolve the assert, go see what field in the struct you have that you dont set here to 
  // the new batch, set that field. Then manually change the value you compare agains sizeof
  // to be able to assert the next time. 
  // Why is it important?
  // It is important to now have bugs, since to have batches working we have to change the code in couple 
  // of places after we add settings to the Batch struct. This place is 1 of them. 
  StaticAssert(sizeof(D_Command_batch) == 80);
  
  D_State* draw_state = d_get_state();
  D_Command_batch* batch = draw_state->command_batch_list.last;
  if ( batch == 0 
    || batch->command_type != command_type  
    || batch->texture      != texture
    || batch->rtv          != __d_get_current_render_target__defaults() 
    || batch->blend_kind   != __d_get_current_blend_kind__defaults()
    || !rect_match(batch->scissor_rect, __d_get_current_scissor_rect__default())
  ) {
    batch = d_add_new_batch(command_type, texture);
  }
  return batch;
}

void d_add_command_to_batch(D_Command_batch* batch, D_Command command)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_node* node = ArenaPush(arena, D_Command_node);
  node->command = command;
  
  QueuePushBack_Name(batch, node, first_command_node, last_command_node, next);
  batch->count += 1;
}

///////////////////////////////////////////////////////////
// - Push/Pops
//
void d_push_blend_kind(D3D_Blend_kind blend_kind)
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_blend_kind_count < ArrayCount(draw_state->arr_of_blend_kinds))
  {
    D3D_Blend_kind current_blend_kind = __d_get_current_blend_kind__defaults();
    if (current_blend_kind != blend_kind)
    {
      draw_state->arr_of_blend_kinds[draw_state->current_blend_kind_count++] = blend_kind;
    }
  }
  else { InvalidCodePath(); }
}

void d_pop_blend_kind()
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_blend_kind_count > 0) { draw_state->current_blend_kind_count -= 1; }
}


D3D_Blend_kind __d_get_current_blend_kind__defaults()
{
  D_State* draw_state = d_get_state();
  D3D_Blend_kind blend_kind = draw_state->defaults.blend_kind;  
  if (draw_state->current_blend_kind_count > 0) { 
    blend_kind = draw_state->arr_of_blend_kinds[draw_state->current_blend_kind_count - 1];
  } 
  return blend_kind;
}

void d_push_render_target(ID3D11RenderTargetView* rtv)
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_render_target_count < ArrayCount(draw_state->arr_of_render_targets))
  {
    ID3D11RenderTargetView* current_rtv = __d_get_current_render_target__defaults();
    if (current_rtv != rtv)
    {
      draw_state->arr_of_render_targets[draw_state->current_render_target_count++] = rtv;
    }
  }
  else { InvalidCodePath(); }
}

void d_pop_render_target()
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_render_target_count > 0) { draw_state->current_render_target_count -= 1; }
}

ID3D11RenderTargetView* __d_get_current_render_target__defaults()
{
  D_State* draw_state = d_get_state();
  ID3D11RenderTargetView* rtv = draw_state->defaults.render_target;  
  if (draw_state->current_render_target_count > 0) { 
    rtv = draw_state->arr_of_render_targets[draw_state->current_render_target_count - 1];
  } 
  return rtv;
}

void d_push_scissor_rect(Rect rect)
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_scissor_rect_count < ArrayCount(draw_state->arr_of_scissor_rects))
  {
    Rect current_rect = __d_get_current_scissor_rect__default();
    if (!rect_match(current_rect, rect)) {
     draw_state->arr_of_scissor_rects[draw_state->current_scissor_rect_count++] = rect;
    }
  }
  else { InvalidCodePath(); }
}

void d_pop_scissor_rect()
{
  D_State* draw_state = d_get_state();
  if (draw_state->current_scissor_rect_count > 0) { draw_state->current_scissor_rect_count -= 1; }
}

Rect __d_get_current_scissor_rect__default()
{
  D_State* draw_state = d_get_state();
  Rect rect = draw_state->defaults.scissor_rect;  
  if (draw_state->current_scissor_rect_count > 0) { 
    rect = draw_state->arr_of_scissor_rects[draw_state->current_scissor_rect_count - 1];
  } 
  return rect;
}

///////////////////////////////////////////////////////////
// - Low level draw commands that know about how the shader works 
//
void d_add_rect_command(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness, V4F32 border_color)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Rect, Null);

  D_Command command = {};
  command.u.rect_c.rect             = rect;
  command.u.rect_c.border_color     = border_color;
  command.u.rect_c.border_thickness = border_thickness;
  command.u.rect_c.softness         = softness;
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i]  = corner_colors[i]; }
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.corner_radius[i] = corner_radiuses.v[i]; }
  d_add_command_to_batch(batch, command);
}

void d_add_texture_command(ID3D11Texture2D* texture, Rect dest_rect, Rect src_rect, B32 is_text, V4F32 text_color)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Texture, texture);

  D_Command command = {};
  command.u.texture_c.dest_rect  = dest_rect;
  command.u.texture_c.src_rect   = src_rect;
  command.u.texture_c.is_text    = is_text;
  command.u.texture_c.text_color = text_color;

  d_add_command_to_batch(batch, command);
}

///////////////////////////////////////////////////////////
// - Higher level draw commands that dont require the caller to know how the shader works 
//
void d_draw_rect(Rect rect, V4F32 color)
{
  V4F32 corner_colors[UV__COUNT] = { color, color, color, color };
  d_add_rect_command(rect, corner_colors, {}, {}, {}, {});
}

void d_draw_rect_pro(Rect rect, V4F32 color_x0y0, V4F32 color_x1y0, V4F32 color_x0y1, V4F32 color_x1y1, V4F32 corner_radii, F32 softness)
{
  V4F32 corner_colors[UV__COUNT] = { color_x0y0, color_x1y0, color_x0y1, color_x1y1 };
  d_add_rect_command(rect, corner_colors, corner_radii, {}, softness, {});
}

void d_draw_rect_inset_borders(Rect rect, V4F32 color, F32 thickness, V4F32 corner_radii, F32 softness)
{
  V4F32 corner_colors[UV__COUNT] = { color, color, color, color };
  d_add_rect_command(rect, corner_colors, corner_radii, thickness, softness, color);
}

void d_draw_rect_outset_borders(Rect rect, V4F32 color, F32 thickness, V4F32 corner_radii, F32 softness)
{
  V4F32 corner_colors[UV__COUNT] = { color, color, color, color };
  d_add_rect_command(rect_padded(rect, thickness), corner_colors, corner_radii, thickness, softness, color);
}

void d_draw_circle(V2F32 center, F32 r, V4F32 color, F32 softness)
{
  Rect rect = rect_from_center(center, v2f32(r, r));
  d_draw_rect_pro(rect, color, color, color, color, v4f32_all(1.0f), softness);
}

void d_draw_circle_inset_border(V2F32 center, F32 r, V4F32 color, F32 thickness, F32 softness)
{
  Rect rect = rect_from_center(center, v2f32(r, r));
  d_draw_rect_inset_borders(rect, color, thickness, v4f32_all(1.0f), softness);
}

#endif