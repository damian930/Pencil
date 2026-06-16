#ifndef DRAW_CPP
#define DRAW_CPP

#include "render/render.h"
#include "render/render.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "draw/draw.h"

///////////////////////////////////////////////////////////
// - State 
//
global D_State __d_g_state = {};

D_State* d_get_state() { return &__d_g_state; }

void d_init()
{
  __d_g_state.arena_for_draw_commands = arena_alloc(Megabytes(64), false, 0);
}

void d_release() 
{ 
  arena_release(&__d_g_state.arena_for_draw_commands);
  __d_g_state = D_State{};
}

///////////////////////////////////////////////////////////
// - Batching 
//
void d_begin_batching(R_Target target) 
{ 
  D_State* draw_state = d_get_state();
  
  draw_state->command_batch_list = {};
  arena_clear(draw_state->arena_for_draw_commands); 

  // There is no need to be doing this here an not inside init, but i am doing this here just because
  draw_state->default_batch_settings.blend_kind    = R_Blend_kind__alpha;
  draw_state->default_batch_settings.render_target = target; 
  draw_state->default_batch_settings.scissor_rect  = rect_make(0.0f, 0.0f, r_get_target_dims(target).x, r_get_target_dims(target).y);
  draw_state->default_batch_settings.fill_mode     = R_Fill_mode__solid;

  // If the assert below is hit, then you have added something to the state.
  // Make sure that things is not a setting stack for batching.
  // If it is, then you have to add a default value and set its value here
  // and also reset the count here to 0.
  StaticAssert(144 == sizeof(D_Command_batch));
}

// todo: THis does not take it target and this makes it less clear about the idea for the call and what it does,
//       make it better 
void d_end_batching() { /*Nothing here*/ }

D_Command_batch_list* d_get_batch_list() { return &d_get_state()->command_batch_list; }

D_Command_batch* d_add_new_batch(D_Command_type command_type, R_Target texture)
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
  StaticAssert(144 == sizeof(D_Command_batch));

  D_Command_batch* new_batch = ArenaPush(arena, D_Command_batch);
  new_batch->command_type = command_type;
  new_batch->texture      = texture;
  new_batch->target       = __d_get_current_render_target__defaults();
  new_batch->scissor_rect = __d_get_current_scissor_rect__defaults();
  new_batch->blend_kind   = __d_get_current_blend_kind__defaults();
  new_batch->fill_mode    = __d_get_current_fill_mode__defaults();

  QueuePushBack_Name(&draw_state->command_batch_list, new_batch, first, last, next_batch);
  draw_state->command_batch_list.count += 1;

  return new_batch;
}

D_Command_batch* d_get_or_add_batch_for_settings(D_Command_type command_type, R_Target texture)
{
  // note:
  // If this static assert fails, that mean that the batch struct has changed. 
  // To resolve the assert, go see what field in the struct you have that you dont set here to 
  // the new batch, set that field. Then manually change the value you compare agains sizeof
  // to be able to assert the next time. 
  // Why is it important?
  // It is important to now have bugs, since to have batches working we have to change the code in couple 
  // of places after we add settings to the Batch struct. This place is 1 of them. 
  StaticAssert(144 == sizeof(D_Command_batch));
  
  D_State* draw_state = d_get_state();
  D_Command_batch* batch = draw_state->command_batch_list.last;
  if ( batch == 0 
    || batch->command_type != command_type  
    || batch->blend_kind   != __d_get_current_blend_kind__defaults()
    || batch->fill_mode    != __d_get_current_fill_mode__defaults()
    || !r_target_match(batch->texture, texture)
    || !r_target_match(batch->target, __d_get_current_render_target__defaults())
    || !rect_match    (batch->scissor_rect, __d_get_current_scissor_rect__defaults())
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
// - Low level draw commands that know about how the shader works 
//
void d_add_rect_command(Rect rect, V4F32 corner_colors[UV__COUNT], V4F32 corner_radiuses, F32 border_thickness, F32 softness, V4F32 border_color)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Rect, r_target_zero_handle());

  D_Command command = {};
  command.u.rect_c.rect             = rect;
  command.u.rect_c.border_color     = border_color;
  command.u.rect_c.border_thickness = border_thickness;
  command.u.rect_c.softness         = softness;
  for EachEnumRange(i, UV, UV__00, UV__COUNT) { command.u.rect_c.vertex_color[i]  = corner_colors[i]; }
  for EachEnumRange(i, UV, UV__00, UV__COUNT) { command.u.rect_c.corner_radius[i] = corner_radiuses.v[i]; }
  d_add_command_to_batch(batch, command);
}

void d_add_texture_command(R_Target texture, Rect dest_rect, Rect src_rect, V4F32 tint)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Texture, texture);

  D_Command command = {};
  command.u.texture_c.dest_rect = dest_rect;
  command.u.texture_c.src_rect  = src_rect;
  command.u.texture_c.tint      = tint;

  d_add_command_to_batch(batch, command);
}

void d_add_line_command(V4F32 color)
{
  D_State* draw_state    = d_get_state();
  Arena* arena           = draw_state->arena_for_draw_commands;
  D_Command_batch* batch = d_get_or_add_batch_for_settings(D_Command_type__Line, r_target_zero_handle());

  D_Command command = {};
  command.u.line_c.color = color;

  d_add_command_to_batch(batch, command);
}

///////////////////////////////////////////////////////////
// - Higher level draw commands that dont require the caller to know how the shader works 
//
void d_fill_with_color(V4F32 color)
{
  V4F32 corner_colors[UV__COUNT] = { color, color, color, color };
  R_Target target                = __d_get_current_render_target__defaults();
  Rect rect                      = rect_make_v(v2f32(0.0f, 0.0f), r_get_target_dims(target));
  d_add_rect_command(rect, corner_colors, {}, {}, {}, {});
}

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
  d_draw_rect_pro(rect, color, color, color, color, (r != 1.0f ? v4f32_all(1.0f) : v4f32_all(0.0f)), softness);
}

void d_draw_circle_inset_border(V2F32 center, F32 r, V4F32 color, F32 thickness, F32 softness)
{
  Rect rect = rect_from_center(center, v2f32(r, r));
  d_draw_rect_inset_borders(rect, color, thickness, v4f32_all(1.0f), softness);
}

void d_draw_texture(R_Target texture, V2F32 pos)
{
  V2F32 texture_dims = r_get_target_dims(texture);
  Rect source_rect   = rect_make_v(v2f32(0.0f, 0.0f), r_get_target_dims(texture));
  Rect dest_rect     = rect_make_v(pos, texture_dims);
  d_add_texture_command(texture, dest_rect, source_rect, white());
}

void d_draw_texture_pro(R_Target texture, Rect dest_rect, Rect source_rect, V4F32 tint)
{
  d_add_texture_command(texture, dest_rect, source_rect, tint);
}

void d_draw_text(Str8 text, FP_Font font, V2F32 pos, V4F32 color)
{
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
    
    d_draw_texture_pro(font.atlas_texture, dest_rect, glyph_data.rect_on_atlas, color);

    F32 advance = glyph_data.advance;
    if (ch_index < text.count - 1)
    {
      FP_Kerning_entry entry = fp_get_kerning(font, ch, text.data[ch_index + 1]);
      if (!IsMemZero(entry)) { advance += entry.advance; }
    } 
    x_offset += advance; 
  }

  #if DEBUG_MODE
  { // Making sure that the x here is the same as in fp to make sure that that we dont do any stupid mistackes
    V2F32 fp_text_dims = fp_measure_text(text, font);
    Assert(fp_text_dims.x == x_offset);
  }
  #endif

  // These are some debug drawings for baseline and stuff
  #if DEBUG_MODE
  {
    // V2F32 fp_text_dims = fp_measure_text(text, font);
    // d_draw_rect(rect_make(pos.x, pos.y, fp_text_dims.x, 1), green());
    // d_draw_rect(rect_make(pos.x, pos.y + font.ascent + font.descent, fp_text_dims.x, 1), green());
    // d_draw_rect(rect_make(pos.x, origin_y, fp_text_dims.x, 1), green());
  }
  #endif
}

void d_draw_text_f(const char* fmt, FP_Font font, V2F32 pos, V4F32 color, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list valist;
  va_start(valist, color);
  Str8 str = str8_valist(scratch.arena, fmt, valist);
  d_draw_text(str, font, pos, color);
  va_end(valist);
  end_scratch(&scratch);
}

///////////////////////////////////////////////////////////
// - Push/Pops
//
#define __D_BatchSettingStackPush_Impl(draw_state, array_var_name, array_count_var_name, new_setting_value) \
  if (draw_state->array_count_var_name < ArrayCount(draw_state->array_var_name)) \
  { \
    draw_state->array_var_name[draw_state->array_count_var_name++] = new_setting_value; \
  } \
  else { InvalidCodePath(); }

#define __D_BatchSettingStackPop_Impl(draw_state, array_var_name, array_count_var_name) \
  if (draw_state->array_count_var_name > 0) { \
    draw_state->array_count_var_name -= 1;  \
  }

#define __D_BatchSettingStackGet_Impl(draw_state, array_var_name, array_count_var_name, default_setting_var_name) \
  if (draw_state->array_count_var_name > 0) {  \
    return draw_state->array_var_name[draw_state->array_count_var_name - 1]; \
  } else { \
    return draw_state->default_batch_settings.default_setting_var_name; \
  } 

void         d_push_blend_kind(R_Blend_kind blend_kind)  { D_State* draw_state = d_get_state(); __D_BatchSettingStackPush_Impl(draw_state, arr_of_blend_kinds, current_blend_kind_count,     blend_kind);  }
void         d_pop_blend_kind()                          { D_State* draw_state = d_get_state(); __D_BatchSettingStackPop_Impl (draw_state, arr_of_blend_kinds, current_blend_kind_count); }
R_Blend_kind __d_get_current_blend_kind__defaults()      { D_State* draw_state = d_get_state(); __D_BatchSettingStackGet_Impl(draw_state, arr_of_blend_kinds,  current_blend_kind_count,     blend_kind);  }

void     d_push_render_target(R_Target target)           { D_State* draw_state = d_get_state(); __D_BatchSettingStackPush_Impl(draw_state, arr_of_render_targets, current_render_target_count,  target);      }
void     d_pop_render_target()                           { D_State* draw_state = d_get_state(); __D_BatchSettingStackPop_Impl (draw_state, arr_of_render_targets, current_render_target_count); }
R_Target __d_get_current_render_target__defaults()       { D_State* draw_state = d_get_state(); __D_BatchSettingStackGet_Impl(draw_state, arr_of_render_targets,  current_render_target_count,  render_target); }

void d_push_scissor_rect(Rect rect)                      { D_State* draw_state = d_get_state(); __D_BatchSettingStackPush_Impl(draw_state, arr_of_scissor_rects, current_scissor_rect_count,  rect);         }
void d_pop_scissor_rect()                                { D_State* draw_state = d_get_state(); __D_BatchSettingStackPop_Impl (draw_state, arr_of_scissor_rects, current_scissor_rect_count);                }
Rect __d_get_current_scissor_rect__defaults()            { D_State* draw_state = d_get_state(); __D_BatchSettingStackGet_Impl(draw_state, arr_of_scissor_rects,  current_scissor_rect_count,  scissor_rect);  }

void        d_push_fill_mode(R_Fill_mode fill_mode)      { D_State* draw_state = d_get_state(); __D_BatchSettingStackPush_Impl(draw_state, arr_of_fill_modes, current_fill_mode_count,  fill_mode);    }
void        d_pop_fill_mode()                            { D_State* draw_state = d_get_state(); __D_BatchSettingStackPop_Impl (draw_state, arr_of_fill_modes, current_fill_mode_count);                   }
R_Fill_mode __d_get_current_fill_mode__defaults()        { D_State* draw_state = d_get_state(); __D_BatchSettingStackGet_Impl(draw_state, arr_of_fill_modes,  current_fill_mode_count,  fill_mode);    }

#endif