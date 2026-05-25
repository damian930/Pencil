#ifndef __UI_CPP
#define __UI_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"

// todo: Struture this file based on the structure of the .h ui file

UI_Context* _ui_g_context    = 0;
UI_Box _ui_g_zero_box        = {};
V2F32 _ui_g_clip_offset_stub = {};

UI_Size ui_size_make(UI_Size_kind kind, F32 value, F32 strictness)
{
  UI_Size size = {};
  size.kind = kind;
  size.value = value; 
  size.strictness = strictness;
  return size;
}
UI_Size ui_px(F32 value)                     { return ui_size_make(UI_Size_kind__px, value, 1.0f); }
UI_Size ui_children_sum()                    { return ui_size_make(UI_Size_kind__children_sum, 0.0f, 0.0f); } // note: value is 0.0f there cause it is actually not used by the implementation
UI_Size ui_text_size()                       { return ui_size_make(UI_Size_kind__text, 0.0f, 1.0f); }         // note: value is 0.0f there cause it is actually not used by the implementation
UI_Size ui_p_of_p(F32 value, F32 strictness) { return ui_size_make(UI_Size_kind__percent_of_parent, value, strictness); }

UI_Context* ui_get_context()
{
  return _ui_g_context;
}

void ui_set_context(UI_Context* context)
{
  _ui_g_context = context;
}

Arena* ui_get_build_arena()
{
  UI_Context* ctx = ui_get_context();
  return ctx->build_arenas[ctx->build_generation % ArrayCount(ctx->build_arenas)];
}

F32 ui_get_mouse_x() { UI_Context* ctx = ui_get_context(); return ctx->mouse_x;  }
F32 ui_get_mouse_y() { UI_Context* ctx = ui_get_context(); return ctx->mouse_y;  }
V2F32 ui_get_mouse_pos() { return v2f32(ui_get_mouse_x(), ui_get_mouse_y()); }


// void ui_set_text_measuring_function(UI_text_measuring_ft* fp)
// {
//   UI_Context* ctx = ui_get_context();
//   ctx->text_measuring_fp = fp;
// }

// UI_text_measuring_ft* ui_get_text_measuring_function()
// {
//   return ui_get_context()->text_measuring_fp;
// }

// V2F32 ui_measure_text(Str8 str)
// {
//   return ui_get_text_measuring_function()(str, ui_get_font(), ui_get_font_size());
// }

// V2F32 ui_measure_text_ex(Str8 str, Font font, F32 font_size)
// {
//   return ui_get_text_measuring_function()(str, font, font_size);
// }


void ui_init()
{
  Arena* arena = arena_alloc(Megabytes(64));
  _ui_g_context = ArenaPush(arena, UI_Context);
  _ui_g_context->context_arena = arena;
  _ui_g_context->style_stacks_arena = arena_alloc(Megabytes(64));
  for EachArrElement(i, _ui_g_context->build_arenas) { 
    _ui_g_context->build_arenas[i] = arena_alloc(Megabytes(64)); 
  }

  _ui_g_context->root_box            = &_ui_g_zero_box;
  _ui_g_context->current_parent_box  = &_ui_g_zero_box;
  _ui_g_context->prev_frame_root_box = &_ui_g_zero_box; 

  _ui_g_context->defaults.flags       = UI_Box_flag__NONE;
  _ui_g_context->defaults.layout_axis = Axis2__y;
  _ui_g_context->defaults.size_x      = ui_children_sum();
  _ui_g_context->defaults.size_y      = ui_children_sum();
  _ui_g_context->defaults.border      = UI_Border{ 0.0f, transparent() };
  _ui_g_context->defaults.softness    = 2.0f;
  _ui_g_context->defaults.font        = {};
  for EachEnumRange(i, UV, UV__00, UV__COUNT) { 
    _ui_g_context->defaults.vertex_colors[i]  = transparent(); 
    _ui_g_context->defaults.corner_radii.v[i] = 0.0f;
  }
}

void ui_release()
{
  for EachArrElement(i, _ui_g_context->build_arenas) {
    arena_release(_ui_g_context->build_arenas[i]);
  }
  arena_release(_ui_g_context->context_arena);
  _ui_g_context = 0;
}

B32 ui_box_is_zero(UI_Box* box)
{
  return ((box == 0) || (box == &_ui_g_zero_box));
}

Str8 ui_get_text_part_from_str8(Str8 id_and_text)
{
  Str8 text = id_and_text;
  RangeU64 range = str8_find(id_and_text, Str8FromC("##"), 0);
  if (range_u64_count(range) > 0) 
  { 
    text = str8_substring(id_and_text, 0, range.min); 
  }
  return text;
}

UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags)
{
  UI_Context* ctx = ui_get_context();
  Arena* arena = ui_get_build_arena();
  
  UI_Box* box = ArenaPush(arena, UI_Box);
  box->id = str8_copy_alloc(ui_get_build_arena(), id_and_text);
  
  flags |= ui_get_flags(); 
  box->flags                   = flags;
  box->layout_axis             = ui_get_layout_axis();    
  box->semantic_size[Axis2__x] = ui_get_size_x();        
  box->semantic_size[Axis2__y] = ui_get_size_y();        
  
  {
    for EachEnumRange(i, UV, UV__00, UV__COUNT) {
      V4F32 vertex_color = ui_get_b_color_uv(i);
      if (!(flags & UI_Box_flag__has_background)) {
        vertex_color = ctx->defaults.vertex_colors[i];
      }
      box->shape_style.vertex_colors[i] = vertex_color;
    }
  }

  {
    UI_Border border = ui_get_border();
    if (!(flags & UI_Box_flag__has_borders)) { border = ctx->defaults.border; }
    box->shape_style.border = border;
  }

  {
    V4F32 corner_r = ui_get_corner_r();
    if (!(flags & UI_Box_flag__has_rounded_corners)) { corner_r = ctx->defaults.corner_radii; }
    box->shape_style.corner_radii = corner_r;
  }

  box->shape_style.softness = ui_get_softness();

  {
    FP_Font font = ctx->defaults.font; 
    if (flags & UI_Box_flag__has_text_contents)
    {
      box->text_style.text = str8_copy_alloc(ui_get_build_arena(), ui_get_text_part_from_str8(id_and_text));
      font = ui_get_font();
    }
    box->text_style.font = font;
  }

  // UI_Box* this_box_prev_frame = ui_get_box_prev_frame(id_and_text);
  // if (!ui_box_is_zero(this_box_prev_frame)) 
  // {
  //   // box->clip_offset = this_box_prev_frame->clip_offset;
  // }

  DllPushBack_Name(ctx->current_parent_box, box, first_child, last_child, next_sibling, prev_sibling);
  box->parent = ui_get_parent();
  box->parent->children_count += 1;

  // Resetting possible single use valus on the style stacks
  ui_pop_single_usage_flags();
  ui_pop_single_usage_layout_axis();
  ui_pop_single_usage_size_x();
  ui_pop_single_usage_size_y();
  ui_pop_single_usage_b_color();
  ui_pop_single_usage_corner_r();
  ui_pop_single_usage_border();
  ui_pop_single_usage_softness();
  ui_pop_single_usage_font();

  return box;
}

void ui_box_set_custom_draw(UI_Box* box, void (*draw_func) (UI_Box*), void* data)
{
  box->custom_draw_func = draw_func; 
  box->custom_draw_data = data; 
}

void ui_push_parent(UI_Box* box)
{
  UI_Context* ctx = ui_get_context();
  box->parent = ctx->current_parent_box; 
  ctx->current_parent_box = box;
}

void ui_pop_parent()
{
  UI_Context* ctx = ui_get_context();
  ctx->current_parent_box = ctx->current_parent_box->parent;
}

UI_Box* ui_get_parent()
{
  return ui_get_context()->current_parent_box;
}

void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos)
{
  Assert(IsMemZero(_ui_g_clip_offset_stub));
  _ui_g_clip_offset_stub = {};

  UI_Context* ctx = ui_get_context();
  
  // Resetting the prev build state
  ctx->flags_stack           = {};
  ctx->layout_axis_stack     = {};
  ctx->semantic_size_x_stack = {};
  ctx->semantic_size_y_stack = {};
  ctx->corner_radius_stack   = {};
  ctx->border_style_stack    = {};
  ctx->softness_stack        = {};
  ctx->text_font_stack       = {};
  for EachEnumRange(i, UV, UV__00, UV__COUNT) { ctx->vertex_color_stacks[i] = {}; }

  ctx->prev_frame_root_box = ctx->root_box;
  ctx->root_box            = &_ui_g_zero_box;
  ctx->current_parent_box  = &_ui_g_zero_box;
  arena_clear(ctx->style_stacks_arena);
  
  // Creating the new build state
  ctx->build_generation += 1;
  Arena* arena = ui_get_build_arena();
  arena_clear(arena);
  
  // Deep copying these since they are allocated on the old build arenas
  ctx->currently_interacted_with_box_id = str8_copy_alloc(ui_get_build_arena(), ctx->currently_interacted_with_box_id);

  // Pushing defaults onto the style stacks
  ui_push_flags(ctx->defaults.flags);
  ui_push_layout_axis(ctx->defaults.layout_axis);
  ui_push_size_x(ctx->defaults.size_x);
  ui_push_size_y(ctx->defaults.size_y);
  ui_push_b_color_uv(UV__00, ctx->defaults.vertex_colors[UV__00]);
  ui_push_b_color_uv(UV__01, ctx->defaults.vertex_colors[UV__01]);
  ui_push_b_color_uv(UV__10, ctx->defaults.vertex_colors[UV__10]);
  ui_push_b_color_uv(UV__11, ctx->defaults.vertex_colors[UV__11]);
  ui_push_corner_r(ctx->defaults.corner_radii);
  ui_push_border(ctx->defaults.border.width, ctx->defaults.border.color);
  ui_push_softness(ctx->defaults.softness);

  ui_set_next_size_x(ui_px(window_dims.x));
  ui_set_next_size_y(ui_px(window_dims.y));
  UI_Box* box = ui_box_make(Str8FromC("## __UI ROOT ELEMENT ID __"), 0);
  ui_push_parent(box);

  ctx->current_parent_box = box;
  ctx->root_box = box;

  ctx->mouse_x = mouse_pos.x;
  ctx->mouse_y = mouse_pos.y;
}

void ui_end_build()
{
  UI_Context* ctx = ui_get_context();
  ui_pop_parent();
  ui_layout_box(ctx->root_box, Axis2__x);
  ui_layout_box(ctx->root_box, Axis2__y);
}

void ui_do_sizing_for_fixed_sized_box(UI_Box* root, Axis2 axis)
{
  switch (root->semantic_size[axis].kind)
  {
    default: { } break;

    case UI_Size_kind__px:
    {
      root->final_on_screen_size.v[axis] = root->semantic_size[axis].value;
      if (f32_is_nan(root->final_on_screen_size.v[axis])) { BP; }
    } break;

    case UI_Size_kind__text:
    {
      V2F32 dims = fp_measure_text(root->text_style.text, root->text_style.font);
      root->final_on_screen_size.v[axis] = dims.v[axis];
    } break;
  }
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_do_sizing_for_fixed_sized_box(child, axis);
  }
}

void ui_do_sizing_for_parent_dependant_box(UI_Box* root, Axis2 axis)
{
  switch (root->semantic_size[axis].kind)
  {
    default: { } break;

    case UI_Size_kind__percent_of_parent:
    {
      UI_Box* parent = root->parent;
      for (;;parent = parent->parent)
      {
        if (parent->semantic_size[axis].kind != UI_Size_kind__children_sum) {
          break;
        } 
        else {
          // note: At this point, if the parent is children size dependant, its size will be 0
          Assert(parent->final_on_screen_size.v[axis] == 0.0f); 
        }
      }
      root->final_on_screen_size.v[axis] = parent->final_on_screen_size.v[axis] * root->semantic_size[axis].value;
    } break;
  }

  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_do_sizing_for_parent_dependant_box(child, axis);
  }
}

void ui_do_sizing_for_child_dependant_box(UI_Box* root, Axis2 axis)
{
  // Setting up sizes for children
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
    ui_do_sizing_for_child_dependant_box(child, axis);
  }
  
  switch (root->semantic_size[axis].kind)
  {
    default: { } break;

    case UI_Size_kind__children_sum:
    {
      for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
      {
        // Floating children dont attribute to the overall size of the parent
        if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 
     
        if (root->layout_axis == axis) {
          root->final_on_screen_size.v[axis] += child->final_on_screen_size.v[axis]; 
        } else {
          root->final_on_screen_size.v[axis] = Max(root->final_on_screen_size.v[axis], child->final_on_screen_size.v[axis]); 
        }
      }

      // Dynamically calculating strictness based on already calculated children
      if (root->layout_axis == axis) 
      {
        F32 children_size_to_maybe_give_out = 0.0f;
        for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
        {
          if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

          F32 child_size = child->final_on_screen_size.v[axis];
          F32 p_to_to_keep = child->semantic_size[axis].strictness;
          F32 p_to_give_out = 1.0f - p_to_to_keep;
          F32 size_to_give_out = child_size * p_to_give_out;
          if (f32_is_nan(size_to_give_out)) { BP; }
          children_size_to_maybe_give_out += size_to_give_out;
        }
        F32 root_size = root->final_on_screen_size.v[axis];
        if (root_size != 0.0f)
        {
          F32 root_p_to_give_out = children_size_to_maybe_give_out / root_size;
          Assert(0.0f <= root_p_to_give_out && root_p_to_give_out <= 1.0f);
          root->semantic_size[axis].strictness = 1.0f - root_p_to_give_out;
        }
      }
      else 
      {
        F32 max_size_after_possible_fixing = 0.0f;
        for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
        {
          if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 

          F32 child_size = child->final_on_screen_size.v[axis];
          F32 p_to_to_keep = child->semantic_size[axis].strictness;
          F32 p_to_give_out = 1.0f - p_to_to_keep;
          F32 size_to_give_out = child_size * p_to_give_out;
          F32 child_size_after_give_out = child_size - size_to_give_out;
          Assert(0.0f <= child_size_after_give_out && child_size_after_give_out <= child_size);
          max_size_after_possible_fixing = Max(max_size_after_possible_fixing, child_size_after_give_out);
        }

        F32 root_size = root->final_on_screen_size.v[axis];
        if (root_size != 0.0f)
        {
          F32 root_p_to_keep = max_size_after_possible_fixing / root_size;
          Assert(0.0f <= root_p_to_keep && root_p_to_keep <= 1.0f);
          root->semantic_size[axis].strictness = root_p_to_keep;
        }
      }

    } break;
  }
}

void ui_do_layout_fixing(UI_Box* root, Axis2 axis)
{
  F32 available_space = root->final_on_screen_size.v[axis];
  if (root->flags & UI_Box_flag__floating_x<<axis)
  {
    available_space = root->parent->final_on_screen_size.v[axis];
  }

  if (root->layout_axis == axis)
  {
    F32 space_used_by_children = 0.0f;
    F32 total_space_children_might_give_out = 0.0f;
    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 

      F32 child_space = child->final_on_screen_size.v[axis];
      space_used_by_children += child_space;
      F32 percent_of_space_child_keeps = child->semantic_size[axis].strictness;
      F32 percent_of_space_child_might_give_out = 1.0f - percent_of_space_child_keeps;
      F32 space_child_might_give = child_space * percent_of_space_child_might_give_out;
      total_space_children_might_give_out += space_child_might_give;
    }
  
    F32 overflow = space_used_by_children - available_space; 
    if (overflow > 0.0f && total_space_children_might_give_out > 0.0f) // We have some room to fix stuff up
    {
      // Fixing every child up relative to how much it might be fixed
      for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
      {
        F32 p_to_keep = child->semantic_size[axis].strictness;
        F32 p_to_give = 1.0f - p_to_keep;
        F32 child_size = child->final_on_screen_size.v[axis];
        F32 space_to_give = child_size * p_to_give;
        F32 space_to_give_proportional = space_to_give / total_space_children_might_give_out;
        F32 space_we_give = space_to_give_proportional * overflow;
  
        child->final_on_screen_size.v[axis] -= space_we_give;
      }
    }
  }
  else 
  {
    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 

      F32 child_space = child->final_on_screen_size.v[axis];
      if (child_space > available_space)
      {
        F32 p_to_keep = child->semantic_size[axis].strictness;
        F32 p_to_give = 1.0f - p_to_keep;
        F32 child_space_to_give_out = child_space * p_to_give;
        child->final_on_screen_size.v[axis] -= child_space_to_give_out;
        if (child->final_on_screen_size.v[axis] < 0.0f) 
        {
          Assert(0, "Not sure about this yet");
          // child->final_on_screen_size.v[axis] = 0.0f;
        }
        if (child->final_on_screen_size.v[axis] < available_space)
        {
          child->final_on_screen_size.v[axis] = available_space;
        }
        
      }
    }
  }

  // Doing children (like Jeffrey Epstein)
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_do_layout_fixing(child, axis);
  }
}

void ui_do_relative_parent_offsets_for_box(UI_Box* root, Axis2 axis)
{
  F32 accumelated_offset = 0.0f;
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    if (child->flags & UI_Box_flag__floating_x<<axis) 
    {
      child->final_parent_offset.v[axis] = 0.0f; // If floating, we leave its start where the parent starts
    }
    else if (root->layout_axis == axis)
    {
      child->final_parent_offset.v[axis] = accumelated_offset;
      accumelated_offset += child->final_on_screen_size.v[axis]; 
    }
    else
    {
      child->final_parent_offset.v[axis] = 0.0f;
    }
    ui_do_relative_parent_offsets_for_box(child, axis);
  }
}

void ui_do_final_rect_for_box(UI_Box* root, Axis2 axis)
{
  static F32 total_offset[Axis2__COUNT];


  if (axis == Axis2__x)
  {
    root->final_on_screen_rect.x = total_offset[axis] + root->final_parent_offset.v[axis];
    root->final_on_screen_rect.width = root->final_on_screen_size.v[axis];
  }
  else if (axis == Axis2__y)
  {
    root->final_on_screen_rect.y = total_offset[axis] + root->final_parent_offset.v[axis];
    root->final_on_screen_rect.height = root->final_on_screen_size.v[axis];
  }
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    F32 prev_total_offset = total_offset[axis]; 
    total_offset[axis] = (axis == Axis2__x ? root->final_on_screen_rect.x : root->final_on_screen_rect.y);
    ui_do_final_rect_for_box(child, axis);
    total_offset[axis] = prev_total_offset;
  }
}

void ui_layout_box(UI_Box* root, Axis2 axis)
{ 
  ui_do_sizing_for_fixed_sized_box(root, axis);
  if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_sizing_for_parent_dependant_box(root, axis);
  if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_sizing_for_child_dependant_box(root, axis);
  if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_layout_fixing(root, axis);

  ui_do_relative_parent_offsets_for_box(root, axis);
  ui_do_final_rect_for_box(root, axis);
}

// note: There might be weird thing going on with ids and text, dont forget about ##
UI_Box* ui_get_box_from_tree(UI_Box* root, Str8 id)
{
  if (ui_box_is_zero(root)) { return &_ui_g_zero_box; }
  if (str8_match(root->id, id, 0)) { return root; }
  
  UI_Box* box = &_ui_g_zero_box;
  for (UI_Box* child = root->first_child; child; child = child->next_sibling)
  {
    if (str8_match(child->id, id, 0))
    {
      box = child;
      break;
    }
    // note: I am not sure, but this shoud be faster
    //       since ui boxed are allocated in depth order
    if (ui_box_is_zero(box)) { box = ui_get_box_from_tree(child, id); } 
    if (!ui_box_is_zero(box)) { break; }
  }
  return box;
}

UI_Box* ui_get_box_prev_frame(Str8 id)
{
  UI_Context* ctx = ui_get_context();
  UI_Box* box = ui_get_box_from_tree(ctx->prev_frame_root_box, id);
  return box;
}

UI_Box_data ui_get_box_data_prev_frame_from_box(UI_Box* box)
{
  return ui_get_box_data_prev_frame_from_id(box->id);
}

UI_Box_data ui_get_box_data_prev_frame_from_id(Str8 id)
{
  UI_Box_data box_data = {};
  UI_Box* box = ui_get_box_prev_frame(id);
  if (!ui_box_is_zero(box)) 
  { 
    box_data.on_screen_rect = box->final_on_screen_rect; 
    box_data.found = true; 
  }
  return box_data;
}

// UI_Box_clip_data ui_get_box_clip_data_prev_frame(Str8 id)
// {
//   UI_Box_clip_data result_data = {};
//   result_data.clip_offset = &_ui_g_clip_offset_stub;
  
//   UI_Box* box = ui_get_box_prev_frame(id);
//   result_data.is_found = !ui_box_is_zero(box);
//   if (result_data.is_found)
//   {
//     result_data.on_screen_dims = rect_dims(box->final_on_screen_rect);
    
//     result_data.content_dims = {};
//     for (UI_Box* child = box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//     {
//       Axis2 axis = box->layout_axis;
//       V2F32 child_dims = rect_dims(child->final_on_screen_rect);
//       result_data.content_dims.v[axis] += child_dims.v[axis];
//       axis = axis2_other(axis);
//       result_data.content_dims.v[axis] = Max(result_data.content_dims.v[axis], child_dims.v[axis]);
//     }

//     result_data.clip_offset = &box->clip_offset;
//   }
//   return result_data;
// } 

B32 ui_is_active_id(Str8 box_id)
{
  return str8_match(ui_get_context()->currently_interacted_with_box_id, box_id, 0); 
}

// void ui_set_active_id(Str8 box_id)
// {
//   UI_Context* context = ui_get_context();
//   context->currently_active_box_id = str8_copy_alloc(ui_get_build_arena(), box_id);
// }

// void ui_reset_active_id_match(Str8 box_id)
// {
//   UI_Context* context = ui_get_context();
//   if (str8_match(context->currently_active_box_id, box_id, 0)) {
//     ui_reset_active();
//   }
// }

B32 ui_is_active_box(UI_Box* box)
{
  return ui_is_active_id(box->id);
}

// void ui_set_active_box(UI_Box* box)
// {
//   return ui_set_active_id(box->id);
// }

// void ui_reset_active_box_match(UI_Box* box)
// {
//   ui_reset_active_id_match(box->id);
// }

B32 ui_has_active()
{
  return str8_match(ui_get_context()->currently_interacted_with_box_id, Str8{}, 0);
}

// void ui_reset_active()
// {
//   ui_get_context()->currently_active_box_id = Str8{};
// }

// todo: This seems like it could be done at the start of the build since there is no dinamic data used here 
UI_Actions ui_actions_from_box(UI_Box* this_frames_box)
{
  UI_Actions* result_actions = &this_frames_box->actions;
  if (this_frames_box->has_been_updated_this_build) { return *result_actions; }
  this_frames_box->has_been_updated_this_build = true;
  result_actions->box = this_frames_box;
  if (this_frames_box->id.count == 0) { return *result_actions; }

  UI_Context* ctx = ui_get_context();

  UI_Box* prev_frame_box = ui_get_box_prev_frame(this_frames_box->id);
  if (ui_box_is_zero(prev_frame_box)) { return {}; }

  // Data to get
  B32 is_hovered = false;
  B32 is_down    = false;
  B32 was_down   = false;
  B32 left_box_while_was_down = false;
  F32 mouse_wheel_move = 0.0f;

  B32 some_other_box_is_being_interacted_with = false;
  if (ctx->currently_interacted_with_box_id.count != 0 && !str8_match(ctx->currently_interacted_with_box_id, prev_frame_box->id, 0))
  {
    some_other_box_is_being_interacted_with = true;
  }
  
  if (is_point_inside_rect(ctx->mouse_x, ctx->mouse_y, prev_frame_box->final_on_screen_rect)) {
    is_hovered = true;
  }

  // Either there is no active box or we are the active box
  // note: Since active box is retained, we just load the retained state of actions,
  //       no need to load hover, we get it each frame just from the box rect.
  if (!some_other_box_is_being_interacted_with)
  {
    was_down                = ctx->currently_interacted_with_box__is_down;
    left_box_while_was_down = ctx->currently_interacted_with_box__left_box_while_was_down;
    // note: is donw is not set, it is calculated next

    if (is_hovered && !was_down)
    {
      if (os_mouse_button_went_down(Mouse_button__left)) // todo: This has a bit of de sync relative to the is_hovered bool since we test if is hovered based on a different mouse pos than the one that was when the mouse went down, most of the time this shoud be fine, but i am not sure about the other times
      {
        // New box is interacted, so setting the state for it
        Assert(!was_down);
        Assert(ctx->currently_interacted_with_box_id.count == 0); 
        Assert(ctx->currently_interacted_with_box__is_down == false);
        Assert(ctx->currently_interacted_with_box__left_box_while_was_down == false);

        is_down = true;
        ctx->currently_interacted_with_box_id = str8_copy_alloc(ui_get_build_arena(), this_frames_box->id);
        ctx->currently_interacted_with_box__is_down = true;
      }
    }
    else if (was_down)
    {
      is_down = was_down;

      if (!is_hovered && is_down) { 
        left_box_while_was_down = true; 
        ctx->currently_interacted_with_box__left_box_while_was_down = true;
      }
  
      if (os_mouse_button_went_up(Mouse_button__left))
      {
        Assert(ctx->currently_interacted_with_box_id.count != 0);
        is_down = false;

        // Resetting retained stuff
        ctx->currently_interacted_with_box_id = Str8{};
        ctx->currently_interacted_with_box__is_down = false;
        ctx->currently_interacted_with_box__left_box_while_was_down = false;
      }
    }
  }

  result_actions->is_hovered = is_hovered;
  result_actions->is_down    = is_down;
  result_actions->was_down   = was_down;
  result_actions->left_box_while_was_down = left_box_while_was_down;
  result_actions->is_clicked = was_down && !is_down && !left_box_while_was_down;
  result_actions->wheel_move = mouse_wheel_move;

  return *result_actions;
}

UI_Actions ui_actions_from_id(Str8 id)
{
  UI_Actions actions = {};
  UI_Box* box = ui_get_box_prev_frame(id);
  if (!ui_box_is_zero(box)) { actions = ui_actions_from_box(box); }
  return actions;
}

///////////////////////////////////////////////////////////
// - Style stacks
//

// note: this is done via memcpy and not =, since in c/cpp = works like memcpy, but it does not work for arrays of fixes size, which i sometimes use, for example for color per vertex, mem cpy makes it work with static fixed size arrays and with values.
#define _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val) \
  node_type* node = ArenaPush(ctx_p->style_stacks_arena, node_type);          \
  node->v = val;                                                              \
  StackPush(&ctx_p->stack_name_inside_ctx, node);                             \
  ctx_p->stack_name_inside_ctx.count += 1;

#define _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type)  \
  if (ctx_p->stack_name_inside_ctx.count > 0) {                          \
    StackPop(&ctx_p->stack_name_inside_ctx);                             \
    ctx_p->stack_name_inside_ctx.count -= 1;                             \
    ctx_p->stack_name_inside_ctx.pop_after_first_use = false;            \
  }

#define _UI_StyleStackGet_Impl(ctx_p, stack_name_inside_ctx, node_type, name_for_default_value_var) \
  if (ctx_p->stack_name_inside_ctx.first != 0) {                                                    \
    return ctx_p->stack_name_inside_ctx.first->v;                                                   \
  } else {                                                                                          \
    return ctx_p->defaults.name_for_default_value_var;                                              \
  }                                                                    

#define _UI_StyleStackSetNext_Impl(ctx_p, stack_name_inside_ctx, node_type, val) \
  if (ctx_p->stack_name_inside_ctx.pop_after_first_use) {                        \
    _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type);             \
  }                                                                              \
  _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val);         \
  ctx_p->stack_name_inside_ctx.pop_after_first_use = true;

#define _U_StyleStackPopSigngleUsage_Imp(ctx_p, stack_name_inside_ctx, node_type) \
  if (ctx->stack_name_inside_ctx.pop_after_first_use) {                           \
    _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type)               \
  }

///////////////////////////////////////////////////////////
// - Default box settings stacks
//
void ui_push_flags(UI_Box_flags v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, flags_stack, UI_Box_flags_node, v) }       
void ui_pop_flags()                    { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, flags_stack, UI_Box_flags_node) } 
void ui_set_next_flags(UI_Box_flags v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, flags_stack, UI_Box_flags_node, v) }       
void ui_pop_single_usage_flags()       { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, flags_stack, UI_Box_flags_node) }
UI_Box_flags ui_get_flags()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, flags_stack, UI_Box_flags_node, flags) }

void ui_push_layout_axis(Axis2 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_pop_layout_axis()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }
void ui_set_next_layout_axis(Axis2 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_pop_single_usage_layout_axis() { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, layout_axis_stack, UI_Layout_axis_node) }
Axis2 ui_get_layout_axis()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, layout_axis) }

void ui_push_size_x(UI_Size v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node, v) }
void ui_pop_size_x()               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node) }
void ui_set_next_size_x(UI_Size v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node, v); }
void ui_pop_single_usage_size_x()  { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, semantic_size_x_stack, UI_Semantic_size_node) }
UI_Size ui_get_size_x()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node, size_x) }

void ui_push_size_y(UI_Size v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node, v) }
void ui_pop_size_y()               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node) }
void ui_set_next_size_y(UI_Size v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node, v) }
void ui_pop_single_usage_size_y()  { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, semantic_size_y_stack, UI_Semantic_size_node) }
UI_Size ui_get_size_y()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node, size_y) }

///////////////////////////////////////////////////////////
// - Style box settings stacks
//
void ui_push_b_color_uv(UV uv, V4F32 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, vertex_color_stacks[uv], UI_Vertex_color_node, v                ) }
void ui_pop_b_color_uv(UV uv)               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl        (ctx, vertex_color_stacks[uv], UI_Vertex_color_node                   ) }
void ui_set_next_b_color_uv(UV uv, V4F32 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl    (ctx, vertex_color_stacks[uv], UI_Vertex_color_node, v                ) }
void ui_pop_single_usage_b_color_uv(UV uv)  { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, vertex_color_stacks[uv], UI_Vertex_color_node                 ) }
V4F32 ui_get_b_color_uv(UV uv)              { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl        (ctx, vertex_color_stacks[uv], UI_Vertex_color_node, vertex_colors[uv]) }

void ui_push_b_color(V4F32 v)      { for EachEnumRange(uv, UV, UV__00, UV__COUNT) { ui_push_b_color_uv(uv, v);     } }
void ui_pop_b_color()              { for EachEnumRange(uv, UV, UV__00, UV__COUNT) { ui_pop_b_color_uv(uv);         } }
void ui_set_next_b_color(V4F32 v)  { for EachEnumRange(uv, UV, UV__00, UV__COUNT) { ui_set_next_b_color_uv(uv, v); } }
void ui_pop_single_usage_b_color() { for EachEnumRange(uv, UV, UV__00, UV__COUNT) { ui_pop_single_usage_b_color_uv(uv); } }

void ui_push_corner_r(V4F32 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, corner_radius_stack, UI_Corner_radius_node, v) }
void ui_pop_corner_r()              { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, corner_radius_stack, UI_Corner_radius_node) }
void ui_set_next_corner_r(V4F32 v)  { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, corner_radius_stack, UI_Corner_radius_node, v) }
void ui_pop_single_usage_corner_r() { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, corner_radius_stack, UI_Corner_radius_node) }
V4F32 ui_get_corner_r()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, corner_radius_stack, UI_Corner_radius_node, corner_radii) }

void ui_push_border(F32 width, V4F32 color)     { UI_Context* ctx = ui_get_context(); UI_Border v = {}; v.width = width; v.color = color; _UI_StyleStackPush_Impl(ctx, border_style_stack, UI_Border_style_node, v) }
void ui_pop_border()                            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, border_style_stack, UI_Border_style_node) }
void ui_set_next_border(F32 width, V4F32 color) { UI_Context* ctx = ui_get_context(); UI_Border v = {}; v.width = width; v.color = color; _UI_StyleStackSetNext_Impl(ctx, border_style_stack, UI_Border_style_node, v) }
void ui_pop_single_usage_border()               { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, border_style_stack, UI_Border_style_node) }
UI_Border ui_get_border()                       { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, border_style_stack, UI_Border_style_node, border) }

void ui_push_softness(F32 softness)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, softness_stack, UI_Softness_node, softness) }
void ui_pop_softness()                  { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, softness_stack, UI_Softness_node) }
void ui_set_next_softness(F32 softness) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, softness_stack, UI_Softness_node, softness) }
void ui_pop_single_usage_softness()     { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, softness_stack, UI_Softness_node) }
F32 ui_get_softness()                   { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, softness_stack, UI_Softness_node, softness) }

///////////////////////////////////////////////////////////
// - Style stack operations for text
//
// void ui_push_text_color(V4F32 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, text_color_stack, UI_Text_color_node, v) }
// void ui_pop_text_color()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, text_color_stack, UI_Text_color_node) }
// void ui_set_next_text_color(V4F32 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, text_color_stack, UI_Text_color_node, v) }
// V4F32 ui_get_text_color()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, text_color_stack, UI_Text_color_node) }

void ui_push_font(FP_Font v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, text_font_stack, UI_Text_font_node, v) }
void ui_pop_font()               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, text_font_stack, UI_Text_font_node) }
void ui_set_next_font(FP_Font v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, text_font_stack, UI_Text_font_node, v) }
void ui_pop_single_usage_font()  { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, text_font_stack, UI_Text_font_node) }
FP_Font ui_get_font()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, text_font_stack, UI_Text_font_node, font) }

// - UI Draw
void ui_draw_box(UI_Box* root, Rect parent_scissor_rect)
{
  #if DEBUG_MODE
  // if (str8_match(root->id, Str8FromC("test id"), 0)) { BP; }
  #endif
  
  // todo: I dont fully like this if here, but for now its like this 
  if (root->custom_draw_func != 0) 
  { 
    root->custom_draw_func(root); 

    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      ui_draw_box(child, parent_scissor_rect);
    }
  }
  else 
  {
    Rect rect = root->final_on_screen_rect;
  
    if (root->flags & UI_Box_flag__has_background)
    {
      d_draw_rect_pro(rect, root->shape_style.vertex_colors[UV__00], root->shape_style.vertex_colors[UV__01], root->shape_style.vertex_colors[UV__10], root->shape_style.vertex_colors[UV__11], root->shape_style.corner_radii, root->shape_style.softness); 
    }

    if (root->flags & UI_Box_flag__has_text_contents)
    {
      d_draw_text(root->text_style.text, root->text_style.font, rect_get_origin(rect), white()); 
    }
    
    if (root->flags & UI_Box_flag__has_borders)
    {
      Rect rect_for_outlines = rect_padded(rect, root->shape_style.border.width);
      // d_draw_rect_inset_borders(rect_for_outlines, root->shape_style.border.color, root->shape_style.border.width, root->shape_style.corner_radii, root->shape_style.softness);
      d_draw_rect_inset_borders(rect, root->shape_style.border.color, root->shape_style.border.width, root->shape_style.corner_radii, root->shape_style.softness);
    }
  
    // Have to scissor ______ (THATS WHAT SHE SAID !!!)
    Rect scissor_rect = parent_scissor_rect;
    if (root->flags & UI_Box_flag__dont_draw_overflow_x || root->flags & UI_Box_flag__dont_draw_overflow_y)
    {
      NotImplemented(); // todo: Fix the innitial rect for this that you pass to the root box that gets drawn
      RangeV2F32 default_scissor_box = {};
      default_scissor_box.min = v2f32((F32)s16_min, (F32)s16_min);
      default_scissor_box.max = v2f32((F32)s16_max, (F32)s16_max);
      
      RangeV2F32 rect_bbox        = range_v2f32_from_rect(rect);
      RangeV2F32 new_scissor_bbox = default_scissor_box;
      
      // Have to make sure that the child scissor is contained within the parent scissor on ax axis, 
      // so a child cant make a scissor larger than the parent and then have its children
      // drawn, though the parent has no overflow flag spcefied.
      // This works per axis. So if no overflow is aplied only for 1 axis, then the other axis shoud be
      // drawn as ussual, with oveflow. This is achieved by having default_scissor_box that extends way pass
      // the ui coordinate limits.  
      if (root->parent->flags & UI_Box_flag__dont_draw_overflow_x || root->parent->flags & UI_Box_flag__dont_draw_overflow_y)
      {
        RangeV2F32 parent_scissor_bbox = range_v2f32_from_rect(parent_scissor_rect);
  
        // Clmaping based to the space that the parent have already limited its children to
        for (U64 _axis = (U64)Axis2__x; _axis < (U64)Axis2__COUNT; _axis += 1)
        {
          Axis2 axis = (Axis2)_axis;
          if (root->parent->flags & (UI_Box_flag__dont_draw_overflow_x<<axis))
          {
            F32 min = rect_bbox.min.v[axis];
            F32 max = rect_bbox.max.v[axis];
    
            if (min < parent_scissor_bbox.min.v[axis]) { min = parent_scissor_bbox.min.v[axis]; }
            if (max > parent_scissor_bbox.max.v[axis]) { max = parent_scissor_bbox.max.v[axis]; }
    
            new_scissor_bbox.min.v[axis] = min; 
            new_scissor_bbox.max.v[axis] = max; 
          }
        }
  
        // The child might have a different axis specified for no overflow, so have to clamp again
        // but this time for the child (root) and not the parent (root->parent)
        for (U64 _axis = (U64)Axis2__x; _axis < (U64)Axis2__COUNT; _axis += 1)
        {
          Axis2 axis = (Axis2)_axis;
          if (root->flags & (UI_Box_flag__dont_draw_overflow_x<<axis))
          {
            if (new_scissor_bbox.min.v[axis] < rect_bbox.min.v[axis]) { new_scissor_bbox.min.v[axis] = rect_bbox.min.v[axis]; }
            if (new_scissor_bbox.max.v[axis] > rect_bbox.max.v[axis]) { new_scissor_bbox.max.v[axis] = rect_bbox.max.v[axis]; }
          }
        }
      }
      else {
        // Simple case, we dont have parent enforce scissoring at all, so we just do it, there are 
        // no additional adjustments we have to do to not mess up what the parent have enforces before us.
        for (U64 _axis = (U64)Axis2__x; _axis < (U64)Axis2__COUNT; _axis += 1)
        {
          Axis2 axis = (Axis2)_axis;
          if (root->flags & (UI_Box_flag__dont_draw_overflow_x<<axis))
          {
            new_scissor_bbox.min.v[axis] = rect_bbox.min.v[axis]; 
            new_scissor_bbox.max.v[axis] = rect_bbox.max.v[axis]; 
          }
        }
      }
  
      scissor_rect = rect_from_range_v2f32(new_scissor_bbox);
      d_push_scissor_rect(scissor_rect);
    }

    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      ui_draw_box(child, scissor_rect);
    }
  
    // No longer scissoring
    if (root->flags & UI_Box_flag__dont_draw_overflow_x || root->flags & UI_Box_flag__dont_draw_overflow_y)
    {
      NotImplemented();
      if (root->parent->flags & UI_Box_flag__dont_draw_overflow_x || root->parent->flags & UI_Box_flag__dont_draw_overflow_y) {
        d_pop_scissor_rect();
      }
    }
  }
}

void ui_draw()
{
  // todo: Dont pass Rect here like this 
  UI_Context* ctx = ui_get_context();
  ui_draw_box(ctx->root_box, Rect{});
}

#endif











