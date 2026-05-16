#ifndef DRAW_CPP
#define DRAW_CPP

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"

///////////////////////////////////////////////////////////
// - Internal builder functions 
//
__D_Rect_builder* __d_builder_func_color(V4F32 color)
{
  D_State* d_state = d_get_state();
  __D_Rect_builder* builder = &d_state->rect_builder;
  for EachEnum(i, UV, UV__00, UV__COUNT) {
    builder->corner_colors[i] = color;
  }
  return builder;
}

D_Command __d_builder_func_get()
{
  D_State* d_state = d_get_state();
  __D_Rect_builder* builder = &d_state->rect_builder;
  D_Command command = {};
  command.u.rect_c.rect  = builder->rect;
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i] = builder->corner_colors[i]; }
  return command;
}

void __d_builder_func_add()
{
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Rect, Null);
  D_Command command      = __d_builder_func_get();
  d_add_command_to_batch(batch, command);
}

///////////////////////////////////////////////////////////
// - State 
//
global D_State __d_g_state = {};

D_State* d_get_state() { return &__d_g_state; }

void d_init()
{
  __d_g_state.arena_for_draw_commands = arena_alloc(Gigabytes(1));

  __d_g_state.rect_builder.color_fp = __d_builder_func_color;
  
  __d_g_state.rect_builder.add = __d_builder_func_add;
  __d_g_state.rect_builder.get = __d_builder_func_get;
}

void d_release() 
{ 
  arena_release(__d_g_state.arena_for_draw_commands);
  __d_g_state = D_State{};
}

///////////////////////////////////////////////////////////
// - Batching 
//
void d_begin_batching() 
{ 
  D_State* draw_state = d_get_state();
  
  draw_state->command_batch_list = {};
  arena_clear(draw_state->arena_for_draw_commands); 
}

void d_end_batching() { /*Nothing here*/ }

D_Command_batch_list* d_get_batch_list() { return &d_get_state()->command_batch_list; }

D_Command_batch* d_add_new_batch(D_Command_type command_type, ID3D11Texture2D* texture)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_batch* new_batch = ArenaPush(arena, D_Command_batch);
  new_batch->command_type = command_type;
  new_batch->texture      = texture;
  
  QueuePushBack_Name(&draw_state->command_batch_list, new_batch, first, last, next_batch);
  draw_state->command_batch_list.count += 1;

  return new_batch;
}

D_Command_batch* d_get_or_add_batch_for_settings(D_Command_type command_type, ID3D11Texture2D* texture)
{
  D_State* draw_state = d_get_state();
  D_Command_batch* batch = draw_state->command_batch_list.last;
  if (batch == 0 || batch->command_type != command_type || batch->texture != texture)
  {
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
// - Draw commands 
//
__D_Rect_builder* d_draw_rect(Rect rect)
{
  D_State* d_state = d_get_state();
  __D_Rect_builder* b = &d_state->rect_builder;
  b->rect = rect;
  return &d_state->rect_builder;
}

void d_add_rect_command_ex(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Rect, Null);

  D_Command command = {};
  command.u.rect_c.rect             = rect;
  command.u.rect_c.border_thickness = border_thickness;
  command.u.rect_c.softness         = softness;
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i] = corner_colors[i]; }
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.corner_radius[i] = corner_radiuses.v[i]; }
  d_add_command_to_batch(batch, command);
}

void d_add_rect_command(Rect rect, V4F32 color)
{
  V4F32 colors[UV__COUNT] = { color, color, color, color };
  d_add_rect_command_ex(rect, colors, {}, {}, {});
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


#endif