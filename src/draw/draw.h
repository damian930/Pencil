#ifndef DRAW_H
#define DRAW_H

#include "render/render.h"

///////////////////////////////////////////////////////////
// - Batching (for now here)
//
enum D_Command_type : U32 {
  D_Command_type__Rect,
  D_Command_type__Texture,
};

struct D_Rect_command {
  Rect rect;
  V4F32 vertex_color[UV__COUNT];
  F32 corner_radius[UV__COUNT];
  F32 border_thickness;
  F32 softness;
};

struct D_Texture_command {
  B32 is_text;
  V4F32 text_color;
  Rect dest_rect;
  Rect src_rect;
};

struct D_Command {
  union {
    D_Rect_command rect_c;
    D_Texture_command texture_c;
  } u;
};

struct D_Command_node {
  D_Command command;
  D_Command_node* next;
};

struct D_Command_batch {
  D_Command_type command_type;

  // Fat stuct data
  ID3D11Texture2D* texture;
  
  D_Command_node* first_command_node;
  D_Command_node* last_command_node;
  U64 count;

  D_Command_batch* next_batch; 
};  

struct D_Command_batch_list {
  D_Command_batch* first;
  D_Command_batch* last;
  U64 count;
};

struct D_State {
  Arena* arena_for_draw_commands;
  D_Command_batch_list command_batch_list;
};

global D_State __d_g_state = {};

D_State* d_get_state() { return &__d_g_state; }

void d_init()
{
  __d_g_state.arena_for_draw_commands = arena_alloc(Gigabytes(1));
}
void d_release() { /*todo:*/ }

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

void d_add_command_to_batch(D_Command_batch* batch, D_Command command)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_node* node = ArenaPush(arena, D_Command_node);
  node->command = command;
  
  QueuePushBack_Name(batch, node, first_command_node, last_command_node, next);
  batch->count += 1;
}

// todo: Test if this api is better for this 
/*
char* result = q.from(&q, "users")
                    ->where_(&q, "age > 18")
                    ->orderBy(&q, "name")
                    ->limit_(&q, 10)
                    ->build(&q);
*/
void d_add_rect_command_ex(Rect rect, V4F32 corner_color[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness)
{
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  D_Command_batch* batch = draw_state->command_batch_list.last;
  if (batch == 0 || batch->command_type != D_Command_type__Rect)
  {
    batch = d_add_new_batch(D_Command_type__Rect, Null);
  }

  D_Command command = {};
  command.u.rect_c.rect             = rect;
  command.u.rect_c.border_thickness = border_thickness;
  command.u.rect_c.softness         = softness;
  for EachEnum(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i] = corner_color[i]; }
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
  D_State* draw_state = d_get_state();
  Arena* arena = draw_state->arena_for_draw_commands;

  // todo: This shoud be its own separate call
  D_Command_batch* batch = draw_state->command_batch_list.last;
  if (batch == 0 || batch->command_type != D_Command_type__Texture || batch->texture != texture)
  {
    batch = d_add_new_batch(D_Command_type__Texture, texture);
  }

  D_Command command = {};
  command.u.texture_c.dest_rect  = dest_rect;
  command.u.texture_c.src_rect   = src_rect;
  command.u.texture_c.is_text    = is_text;
  command.u.texture_c.text_color = text_color;

  d_add_command_to_batch(batch, command);
}

#endif