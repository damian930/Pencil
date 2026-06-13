#ifndef __UI_CPP
#define __UI_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"

UI_Context* __ui_g_context    = 0;
UI_Box __ui_g_zero_box        = {};

UI_Size ui_size_make(UI_Size_kind kind, F32 value, F32 strictness)
{
  UI_Size size = {};
  size.kind       = kind;
  size.value      = value; 
  size.strictness = strictness;
  return size;
}

UI_Size ui_px(F32 value)                 { return ui_size_make(UI_Size_kind__px, value, 1.0f); }
UI_Size ui_fit()                         { return ui_size_make(UI_Size_kind__fit, 0.0f, 0.0f); } 
UI_Size ui_text_size()                   { return ui_size_make(UI_Size_kind__text, 0.0f, 1.0f); }         
UI_Size ui_p_of_p(F32 p, F32 strictness) { return ui_size_make(UI_Size_kind__percent_of_parent, p, strictness); }         

UI_Context* ui_get_context()
{
  return __ui_g_context;
}

void ui_set_context(UI_Context* context)
{
  __ui_g_context = context;
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
  __ui_g_zero_box.first_child  = &__ui_g_zero_box;
  __ui_g_zero_box.last_child   = &__ui_g_zero_box;
  __ui_g_zero_box.next_sibling = &__ui_g_zero_box;
  __ui_g_zero_box.prev_sibling = &__ui_g_zero_box;
  __ui_g_zero_box.parent       = &__ui_g_zero_box;

  Arena* arena = arena_alloc(Megabytes(64), false, 0);
  __ui_g_context = ArenaPush(arena, UI_Context);
  __ui_g_context->context_arena = arena;
  __ui_g_context->style_stacks_arena = arena_alloc(Megabytes(64), false, 0);
  for EachArrElement(i, __ui_g_context->build_arenas) { 
    __ui_g_context->build_arenas[i] = arena_alloc(Megabytes(64), true, (i == 0 ? 1 : 44444)); // TODO: The stable address here shoud be removed 
  }

  __ui_g_context->root_box            = &__ui_g_zero_box;
  __ui_g_context->current_parent_box  = &__ui_g_zero_box;
  __ui_g_context->prev_frame_root_box = &__ui_g_zero_box; 

  __ui_g_context->defaults.flags        = UI_Box_flag__NONE;
  __ui_g_context->defaults.layout_axis  = Axis2__y;
  __ui_g_context->defaults.size_x       = ui_fit();
  __ui_g_context->defaults.size_y       = ui_fit();
  __ui_g_context->defaults.border_width = 0.0f;
  __ui_g_context->defaults.border_color = transparent();
  __ui_g_context->defaults.padding      = 0.0f;
  __ui_g_context->defaults.child_gap    = 0.0f;
  for EachEnumRange(i, UV, UV__00, UV__COUNT) {
    __ui_g_context->defaults.vertex_colors[i]  = transparent();
    __ui_g_context->defaults.corner_radii.v[i] = 0.0f;
  }
  __ui_g_context->defaults.softness     = 0.0f;
  __ui_g_context->defaults.font         = {};
}

void ui_release()
{
  for EachArrElement(i, __ui_g_context->build_arenas) {
    arena_release(&__ui_g_context->build_arenas[i]);
  }
  arena_release(&__ui_g_context->context_arena);
  __ui_g_context = 0;
}

B32 ui_box_is_zero(UI_Box* box)
{
  return ((box == 0) || (box == &__ui_g_zero_box));
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
  box->padding                 = ui_get_padding();
  box->child_gap               = ui_get_child_gap();
  
  {
    V4F32 border_color = ui_get_border_color();
    F32 border_width = ui_get_border_width();
    if (!(flags & UI_Box_flag__has_borders)) { 
      box->border_color = border_color;
      box->border_width = border_width;
    }
  }

  {
    for EachEnumRange(i, UV, UV__00, UV__COUNT) {
      V4F32 vertex_color = ui_get_b_color_uv(i);
      if (!(flags & UI_Box_flag__has_background)) {
        vertex_color = ctx->defaults.vertex_colors[i];
      }
      box->vertex_colors[i] = vertex_color;
    }
  }

  {
    V4F32 corner_r = ui_get_corner_r();
    if (!(flags & UI_Box_flag__has_rounded_corners)) { corner_r = ctx->defaults.corner_radii; }
    box->corner_radii = corner_r;
  }

  box->softness = ui_get_softness();

  {
    FP_Font font = ctx->defaults.font; 
    if (flags & UI_Box_flag__has_text_contents)
    {
      box->text = str8_copy_alloc(ui_get_build_arena(), ui_get_text_part_from_str8(id_and_text));
      font = ui_get_font();
    }
    box->font = font;
  }

  DllPushBack_Name_NullFunc(ctx->current_parent_box, box, first_child, last_child, next_sibling, prev_sibling, ui_box_is_zero);
  box->parent = ui_get_parent();
  box->parent->children_count += 1;
  box->first_child  = &__ui_g_zero_box;
  box->last_child   = &__ui_g_zero_box;
  box->next_sibling = &__ui_g_zero_box;
  box->prev_sibling = &__ui_g_zero_box;

  // Resetting possible single use valus on the style stacks
  ui_pop_single_usage_flags();
  ui_pop_single_usage_layout_axis();
  ui_pop_single_usage_size_x();
  ui_pop_single_usage_size_y();
  ui_pop_single_usage_padding();
  ui_pop_single_usage_b_color();
  ui_pop_single_usage_corner_r();
  ui_pop_single_usage_softness();
  ui_pop_single_usage_font();

  return box;
}

UI_Box* ui_box_make_f(const char* fmt, UI_Box_flags flags, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list args;
  va_start(args, flags);
  Str8 str = str8_valist(scratch.arena, fmt, args);
  UI_Box* box = ui_box_make(str, flags);
  va_end(args);
  end_scratch(&scratch);
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

// note: This was used to find the most inner child that is hovered to set hot_box when we had hot boxes
// 
// UI_Box* find_hoverd_child_for_box(UI_Box* box)
// {
//   if (ui_box_is_zero(box)) { return &__ui_g_zero_box; }

//   // Hover is on this substree
//   UI_Box* result_box = &__ui_g_zero_box;
//   if (range_v2f32_within(box->final_on_screen_bbox, ui_get_mouse_pos()))
//   {
//     if (box->id.count != 0 && box->flags & UI_Box_flag__hoverable) { result_box = box; }
//     for (UI_Box* child = box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//     {
//       UI_Box* hovered_box_inside_children = find_hoverd_child_for_box(child);
//       if (!ui_box_is_zero(hovered_box_inside_children)) { result_box = hovered_box_inside_children; }
//     }
//   }

//   return result_box;
// }

UI_Box* next_box_in_subtree_depth_first(UI_Box* start_box, B32 include_start_box)
{
  // Going over the sub-tree that start with start_box at the root
  if (ui_box_is_zero(start_box)) { return &__ui_g_zero_box; }
  if (start_box->id.count != 0 && include_start_box) { return start_box; }
  for (UI_Box* child = start_box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    if (child->id.count != 0) { return child; }
    UI_Box* possible_box = next_box_in_subtree_depth_first(child, false);
    if (possible_box->id.count != 0) { return possible_box; }
  }
  return &__ui_g_zero_box;
}

UI_Box* need_a_name(UI_Box* start_box)
{
  if (ui_box_is_zero(start_box)) { return &__ui_g_zero_box; }
  
  UI_Box* test_box = next_box_in_subtree_depth_first(start_box, false);
  if (!ui_box_is_zero(test_box)) { return test_box; }

  for (UI_Box* sibling = start_box->next_sibling; !ui_box_is_zero(sibling); sibling = sibling->next_sibling)
  {
    test_box = next_box_in_subtree_depth_first(sibling, true);
    if (!ui_box_is_zero(test_box)) { return test_box; }
  }

  return need_a_name(start_box->parent->next_sibling);
}

void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos)
{
  UI_Context* ctx = ui_get_context();
  
  // Resetting the prev build state
  __ui_g_context->flags_stack          = {};
  __ui_g_context->layout_axis_stack    = {};
  __ui_g_context->semantic_size_x_stack = {};
  __ui_g_context->semantic_size_y_stack = {};
  __ui_g_context->border_width_stack    = {};
  __ui_g_context->border_color_stack   = {};
  __ui_g_context->padding_stack        = {};
  __ui_g_context->child_gap_stack      = {};
  for EachEnumRange(i, UV, UV__00, UV__COUNT) { __ui_g_context->vertex_color_stacks[i] = {}; }
  __ui_g_context->corner_radius_stack  = {};
  __ui_g_context->softness_stack       = {};
  __ui_g_context->text_font_stack      = {};

  ctx->prev_frame_root_box = ctx->root_box;
  ctx->root_box            = &__ui_g_zero_box;
  ctx->current_parent_box  = &__ui_g_zero_box;
  arena_clear(ctx->style_stacks_arena);
  
  // Creating the new build state
  ctx->build_generation += 1;
  Arena* arena = ui_get_build_arena();
  arena_clear(arena);
  
  // Deep copying these since they are allocated on the old build arena
  ctx->interacted_with_box_id = str8_copy_alloc(ui_get_build_arena(), ctx->interacted_with_box_id); 
  ctx->active_box_id          = str8_copy_alloc(ui_get_build_arena(), ctx->active_box_id); 
  ctx->navigated_box_id       = str8_copy_alloc(ui_get_build_arena(), ctx->navigated_box_id); 

  for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__tab && ev->key_event.went_down) 
    {
      if (ctx->navigated_box_id.count == 0) { 
        ctx->navigated_box_id = ctx->prev_frame_root_box->id; 
      }
      else {
        UI_Box* current_nav_box   = ui_get_box_prev_frame(ctx->navigated_box_id);          
        UI_Box* test_next_nav_box = need_a_name(current_nav_box);
        ctx->navigated_box_id     = test_next_nav_box->id;
      }
        
      os_consume_frame_event(ev);
      break;
    }
  }

  // Pushing defaults onto the style stacks
  ui_push_flags(ctx->defaults.flags);
  ui_push_layout_axis(ctx->defaults.layout_axis);
  ui_push_size_x(ctx->defaults.size_x);
  ui_push_size_y(ctx->defaults.size_y);
  ui_push_padding(ctx->defaults.padding);
  ui_push_child_gap(ctx->defaults.child_gap);
  ui_push_border_width(ctx->defaults.border_width);
  ui_push_border_color(ctx->defaults.border_color);
  ui_push_b_color_uv(UV__00, ctx->defaults.vertex_colors[UV__00]);
  ui_push_b_color_uv(UV__01, ctx->defaults.vertex_colors[UV__01]);
  ui_push_b_color_uv(UV__10, ctx->defaults.vertex_colors[UV__10]);
  ui_push_b_color_uv(UV__11, ctx->defaults.vertex_colors[UV__11]);
  ui_push_corner_r(ctx->defaults.corner_radii);
  ui_push_softness(ctx->defaults.softness);
  // TODO: There is no way to push font here since we dont have a default one yet

  ui_set_next_size_x(ui_px(window_dims.x));
  ui_set_next_size_y(ui_px(window_dims.y));
  UI_Box* box = ui_box_make(Str8FromC("## __UI ROOT ELEMENT ID __"), 0);
  ui_push_parent(box);

  ctx->current_parent_box = box;
  ctx->root_box = box;

  ctx->mouse_x = mouse_pos.x;
  ctx->mouse_y = mouse_pos.y;

  ctx->final_cursor = OS_Cursor__arrow;
}

void ui_end_build()
{
  UI_Context* ctx = ui_get_context();
  ui_pop_parent();
  __ui_layout_box(ctx->root_box, Axis2__x);
  __ui_layout_box(ctx->root_box, Axis2__y);
  os_set_cursor(ctx->final_cursor);
}

///////////////////////////////////////////////////////////
// - Layout algorithm
//
// ========
// | algo:
// | - figure out sizes for each box
// |   - size fixed sized boxes
// |   - % of parent
// |   - size children dependant boxes
// |   - layout fixing 
// | - position each box, this is just relative to its parent
// | - create final bounding boxes 

void __ui_do_sizing_for_fixed_sized_box(UI_Box* root, Axis2 axis)
{
  switch (root->semantic_size[axis].kind)
  {
    default: {} break;

    case UI_Size_kind__px:
    {
      root->final_on_screen_size.v[axis] = root->semantic_size[axis].value;
    } break;

    case UI_Size_kind__text:
    {
      V2F32 dims = fp_measure_text(root->text, root->font);
      root->final_on_screen_size.v[axis] = dims.v[axis];
    } break;
  }
  
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
    __ui_do_sizing_for_fixed_sized_box(child, axis);
  }
}

void __ui_do_sizing_for_parent_dependant_box(UI_Box* root, Axis2 axis)
{
  if (root->semantic_size[axis].kind == UI_Size_kind__percent_of_parent)
  {
    UI_Box* first_non_child_dependant_parent = &__ui_g_zero_box;
    for (UI_Box* ancestor = root->parent; !ui_box_is_zero(ancestor); ancestor = ancestor->parent) {
      if (ancestor->semantic_size[axis].kind != UI_Size_kind__fit) {
        first_non_child_dependant_parent = ancestor;
        break; 
      }
    }
    Assert(!ui_box_is_zero(first_non_child_dependant_parent)); // The top ui box is fixed sized, so we shoud get this allways

    F32 parent_size = first_non_child_dependant_parent->final_on_screen_size.v[axis];
    clamp_f32_inplace(&root->semantic_size[axis].value, 0.0f, 1.0f);
    root->final_on_screen_size.v[axis] = parent_size * root->semantic_size[axis].value;
  }
  
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
    __ui_do_sizing_for_parent_dependant_box(child, axis);
  }
}

void __ui_do_sizing_for_children_dependant_box(UI_Box* root, Axis2 axis)
{
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
    __ui_do_sizing_for_children_dependant_box(child, axis);
  }

  switch (root->semantic_size[axis].kind)
  {
    default: {} break;

    case UI_Size_kind__fit:
    {
      for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
      {
        if (root->layout_axis == axis) { root->final_on_screen_size.v[axis] += child->final_on_screen_size.v[axis]; }
        else { root->final_on_screen_size.v[axis] = Max(root->final_on_screen_size.v[axis], child->final_on_screen_size.v[axis]); }
      }
    } break;
  }
}

void __ui_do_layout_size_fixing(UI_Box* root, Axis2 axis)
{
  // If the inner contents of the root are larger then the root alowes, 
  // then we make the smaller here based on the strictness if possible.

  // todo: For layout axis we make all the children smaller
  // but for non layout we just make the box smaller

  // if (str8_match(root->id, Str8FromC("Wrapper"), 0)) { BP; }

  if (root->layout_axis == axis)
  {
    F32 total_size_of_children  = 0.0f;
    F32 strictness_total_budget = 0.0f;
    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      total_size_of_children += child->final_on_screen_size.v[axis];
      strictness_total_budget += (1.0f - child->semantic_size[axis].strictness);
    }
  
    // We have overflow of children here, might need fixing
    F32 root_size = root->final_on_screen_size.v[axis];
    if (root_size < total_size_of_children)
    {
      for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
      {
        F32 percentage_of_removable_size    =  1.0f - child->semantic_size[axis].strictness;
        F32 ratio_relative_to_total_budget  =  percentage_of_removable_size / strictness_total_budget;
        F32 amount_to_remove                =  (total_size_of_children - root_size) * ratio_relative_to_total_budget;
        child->final_on_screen_size.v[axis] -= amount_to_remove;
      }
    }
  }
  else  
  {
    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      F32 child_max_legal_size = root->final_on_screen_size.v[axis];
      F32 child_size = child->final_on_screen_size.v[axis];
      if (child_size > child_max_legal_size)
      {
        F32 removable_budget  = 1.0f - child->semantic_size[axis].strictness;
        F32 overflow          = child_size - child_max_legal_size;
        F32 possible_new_size = child->final_on_screen_size.v[axis] - overflow;
        F32 legal_min_size    = child->final_on_screen_size.v[axis] * child->semantic_size[axis].strictness;
        child->final_on_screen_size.v[axis] = Max(legal_min_size, possible_new_size);
      }
    }
  }

  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    __ui_do_layout_size_fixing(child, axis);
  }
}

void __ui_do_relative_parent_offsets_for_box(UI_Box* root, Axis2 axis)
{
  U64 child_index = 0;
  F32 accumelated_children_sizes = 0.0f;
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling, child_index += 1)
  {
    // Floating doesnt have offset from parent
    if (child->flags & UI_Box_flag__floating_x<<axis) {}
    else 
    {
      if (axis == root->layout_axis) {
        child->final_parent_offset.v[axis] += accumelated_children_sizes;
        accumelated_children_sizes += child->final_on_screen_size.v[axis];
      }
    }

    // Offsetting the boxes based on their parent clip value
    // TODO: I have no idea if this works still
    // if (child->flags & UI_Box_flag__floating_x<<axis) {} /*|| (root->flags & UI_Box_flag__aply_clip_offset_on_clildren_floating)*/
    // else {
    //   child->final_parent_offset.v[axis] += root->clip_offset[axis];
    // }

    __ui_do_relative_parent_offsets_for_box(child, axis);
  }
}

void __ui_do_final_rect_for_box(UI_Box* root, Axis2 axis, RangeV2F32 parent_clip_bbox)
{
  static F32 total_offset[Axis2__COUNT] = {};

  // Positioning boxes regardless of clip
  root->final_on_screen_bbox.min.v[axis] = total_offset[axis] + root->final_parent_offset.v[axis];
  root->final_on_screen_bbox.max.v[axis] = root->final_on_screen_bbox.min.v[axis] + root->final_on_screen_size.v[axis];

  // Dealing with clip rects
  root->clip_bbox.min.v[axis] = parent_clip_bbox.min.v[axis]; 
  root->clip_bbox.max.v[axis] = parent_clip_bbox.max.v[axis]; 
  //
  RangeV2F32 new_clip_bbox = parent_clip_bbox;
  {
    if (root->flags & UI_Box_flag__clip_x<<axis) {
      new_clip_bbox = intersect_range_v2f32_on_axis(parent_clip_bbox, root->final_on_screen_bbox, axis);
    }
  }

  // Doing children
  F32 children_size_sum = 0.0f;
  U64 child_index = 0;
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling, child_index += 1)
  {
    if (root->layout_axis == axis) { children_size_sum += child->final_on_screen_size.v[axis] + (child_index * root->child_gap); }
    else { children_size_sum = Max(children_size_sum, child->final_on_screen_size.v[axis]); }

    F32 prev_total_offset = total_offset[axis]; 
    total_offset[axis] = root->final_on_screen_bbox.min.v[axis];
    __ui_do_final_rect_for_box(child, axis, new_clip_bbox);
    total_offset[axis] = prev_total_offset;
  }
  root->inner_content_dims.v[axis] = children_size_sum;
}

void __ui_layout_box(UI_Box* root, Axis2 axis)
{ 
  // Sizing
  __ui_do_sizing_for_fixed_sized_box(root, axis);     
  __ui_do_sizing_for_parent_dependant_box(root, axis); 
  __ui_do_sizing_for_children_dependant_box(root, axis);      
  
  // Fixing
  __ui_do_layout_size_fixing(root, axis);

  // Final positioning
  __ui_do_relative_parent_offsets_for_box(root, axis);

  // TODO: 1000 is a bit too little here
  RangeV2F32 parent_clip_bbox = range_v2f32(v2f32(-1000.0f, -1000.0f), v2f32(1000.0f, 1000.0f));
  __ui_do_final_rect_for_box(root, axis, parent_clip_bbox);
}

// Prev iteration of the ui system layout algorightm
/*
void ui_do_sizing_for_fixed_sized_box(UI_Box* root, Axis2 axis)
{
  switch (root->semantic_size[axis].kind)
  {
    default: { } break;

    case UI_Size_kind__px:
    {
      root->final_on_screen_size.v[axis] = root->semantic_size[axis].value + (2*root->padding);
    } break;

    case UI_Size_kind__text:
    {
      V2F32 dims = fp_measure_text(root->text_style.text, root->text_style.font);
      root->final_on_screen_size.v[axis] = dims.v[axis] + (2*root->padding);
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
    default: {} break;

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
      root->final_on_screen_size.v[axis] = (parent->final_on_screen_size.v[axis] * root->semantic_size[axis].value) + (2*root->padding);
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
          root->final_on_screen_size.v[axis] += child->final_on_screen_size.v[axis] + (2*root->padding); 
        } else {
          root->final_on_screen_size.v[axis] = Max(root->final_on_screen_size.v[axis], child->final_on_screen_size.v[axis]) + (2*root->padding); 
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
  // Clip boxes represent something like a viewport, for that reason clip offset exists.
  // That means that immediate children for a clip box are not to be made smaller in size
  // by the layout fixing codepath, though their children work as ussual.
  if (!(root->flags & UI_Box_flag__clip_x<<axis))
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
  }

  // Doing children (like Jeffrey Epstein)
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_do_layout_fixing(child, axis);
  }
}

void ui_do_relative_parent_offsets_for_box(UI_Box* root, Axis2 axis)
{
  // if (root->padding != 0.0f && axis == Axis2__y) { BP; }
  F32 accumelated_children_offset = 0.0f;
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    if (!(child->flags & UI_Box_flag__floating_x<<axis))
    {
      if (root->layout_axis == axis) { 
        child->final_parent_offset.v[axis] = root->padding + accumelated_children_offset; 
        accumelated_children_offset += child->final_on_screen_size.v[axis]; 
      }
      else { child->final_parent_offset.v[axis] = root->padding + accumelated_children_offset; }
    }

    if (!(child->flags & UI_Box_flag__floating_x<<axis) || (root->flags & UI_Box_flag__aply_clip_offset_on_clildren_floating))
    {
      child->final_parent_offset.v[axis] += root->clip_data.clip_value[axis];
    }

    ui_do_relative_parent_offsets_for_box(child, axis);
  }
}

void ui_do_final_rect_for_box(UI_Box* root, Axis2 axis, RangeV2F32 parent_clip_bbox)
{
  static F32 total_offset[Axis2__COUNT]  = {};

  // Positioning boxes regardless of clip
  root->final_on_screen_bbox.min.v[axis] = total_offset[axis] + root->final_parent_offset.v[axis];
  root->final_on_screen_bbox.max.v[axis] = root->final_on_screen_bbox.min.v[axis] + root->final_on_screen_size.v[axis];

  // Dealing with clip rects
  root->clip_data.clip_bbox.min.v[axis] = parent_clip_bbox.min.v[axis]; 
  root->clip_data.clip_bbox.max.v[axis] = parent_clip_bbox.max.v[axis]; 
  //
  RangeV2F32 new_clip_bbox = parent_clip_bbox;
  {
    if (root->flags & UI_Box_flag__clip_x<<axis) {
      new_clip_bbox = intersect_range_v2f32_on_axis(parent_clip_bbox, root->final_on_screen_bbox, axis);
    }
  }

  // Doing children
  F32 children_size_sum = 0.0f;
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    if (root->layout_axis == axis) { children_size_sum += child->final_on_screen_size.v[axis]; }
    else { children_size_sum = Max(children_size_sum, child->final_on_screen_size.v[axis]); }

    F32 prev_total_offset = total_offset[axis]; 
    total_offset[axis] = root->final_on_screen_bbox.min.v[axis];
    ui_do_final_rect_for_box(child, axis, new_clip_bbox);
    total_offset[axis] = prev_total_offset;
  }
  root->inner_content_dims.v[axis] = children_size_sum;
}

void ui_layout_box(UI_Box* root, Axis2 axis)
{ 
  ui_do_sizing_for_fixed_sized_box(root, axis);      if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_sizing_for_parent_dependant_box(root, axis); if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_sizing_for_child_dependant_box(root, axis);  if (f32_is_nan(root->final_on_screen_size.x) || f32_is_nan(root->final_on_screen_size.y)) { BP; }
  ui_do_layout_fixing(root, axis);

  ui_do_relative_parent_offsets_for_box(root, axis);

  // todo: After moving to ranges i can have clear bound be the same for everything
  //       For now using these values
  RangeV2F32 parent_clip_bbox = range_v2f32(v2f32(-1000.0f, -1000.0f), v2f32(1000.0f, 1000.0f));
  ui_do_final_rect_for_box(root, axis, parent_clip_bbox);
}
*/

///////////////////////////////////////////////////////////
// - Box data stuff
//

// note: There might be weird thing going on with ids and text, dont forget about ##
UI_Box* ui_get_box_from_tree(UI_Box* root, Str8 id)
{
  if (id.count == 0)               { return &__ui_g_zero_box; }
  if (ui_box_is_zero(root))        { return &__ui_g_zero_box; }
  if (str8_match(root->id, id, 0)) { return root; }
  
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    // note: I am not sure, but this shoud be faster
    //       since ui boxed are allocated in depth order
    if (str8_match(child->id, id, 0)) { return child; }
    else { 
      UI_Box* box = ui_get_box_from_tree(child, id); 
      if (!ui_box_is_zero(box)) { return box; }
    }
  }
  return &__ui_g_zero_box;
}

UI_Box* ui_get_box_prev_frame(Str8 id)
{
  UI_Context* ctx = ui_get_context();
  UI_Box* box = ui_get_box_from_tree(ctx->prev_frame_root_box, id);
  return box;
}

UI_Box_data ui_box_data_from_box_prev_frame(UI_Box* box)
{
  return ui_box_data_from_box_id_prev_frame(box->id);
}

UI_Box_data ui_box_data_from_box_id_prev_frame(Str8 id)
{
  UI_Box_data box_data = {};
  UI_Box* box = ui_get_box_prev_frame(id);
  if (!ui_box_is_zero(box)) 
  { 
    box_data.is_found           = true; 
    box_data.on_screen_bbox     = box->final_on_screen_bbox; 
    box_data.inner_content_dims = box->inner_content_dims;
    box_data.clip_offset        = v2f32(box->clip_offset[Axis2__x], box->clip_offset[Axis2__y]) ;
  }
  return box_data;
}

/* IDEAS ABOUT ACTIONS FOR POSSIBLE LATER:
  - Right now hover and active work very simply. The caller just asks for the events,
    the call then checks the boxe's rect and does hover and if mosue is down the active logic.
    This is not the best way to do this. 
    Here are some cases when this fais:
      A box with another box that has id and when we press inner box we would like the outer box to be active.
      This cant be done, since the inner box will get active and we dont have a way to bubble or propogate.
      ---
      A box with inner box and we do hover effect and we dont want to do if any child is hoverd, then we cant know it,
      since the hover for the outer box will be done first and it will hover and then the child and it will hover, 
      there is no way to opt in our out of thi.
      ---
    This might be solved by having the default way of inputs, either the deepest child that has id or interactable 
    or what we do right now, just by the rect. But then we would have to have some modifiers that would change the logic.
    One way to do this would be to have a flag that makes the box hot if any immediate child is hot.
    Another is if any inner child is hot. 

    I am not sure where i need this yet and how example, so will just leave this here for now like this,
    but i am writing this here for reasons to remind me of this all if i ever need a more specific logic
    for hover and active.
*/
UI_Actions ui_actions_from_box(UI_Box* this_frames_box)
{
  // TODO: You have to use a scissor rect here and then to recursive intersections if any parent in the tree for the box has clip flags set

  UI_Actions* result_actions = &this_frames_box->actions;
  if (this_frames_box->has_been_updated_this_build) { return *result_actions; }
  
  this_frames_box->has_been_updated_this_build = true;
    
  // We dont update a box that doesnt have id on it
  if (this_frames_box->id.count == 0) { return *result_actions; } 
      
  // We dont update boxes that are created this frame and were not present last frame
  UI_Box* prev_frames_box = ui_get_box_prev_frame(this_frames_box->id);
  if (ui_box_is_zero(prev_frames_box)) { Assert(IsZeroStruct(*result_actions)); *result_actions; } 

  UI_Context* ctx = ui_get_context();

  // Data to get
  B32 is_hovered              = false;
  B32 is_down                 = false;
  B32 was_down                = false;
  B32 left_box_while_was_down = false;
  B32 is_active               = false;
  B32 is_navigated            = false;

  B32 some_other_box_is_being_interacted_with = (
    ctx->interacted_with_box_id.count != 0 // There is a box that is interacted with right now
    &&
    !str8_match(ctx->interacted_with_box_id, prev_frames_box->id, 0) // We are not the box that is interacted with right now
  );

  // TODO: Here only usethe visible rect for boxes 
  //       This should know to not use the boxes inside clip elements that are clipped of the screen
  //       Right now this is not done, cause i am taking care of other things and cant test this yet.
  is_hovered = range_v2f32_within(prev_frames_box->final_on_screen_bbox, ui_get_mouse_pos());

  // Either there is no active box or we are the active box
  // Since interacted box data is retained across frame boundary, 
  // we just load the retained state and possibly update it here.
  // No need to load hover, we get it each frame just from the box rect.
  if (!some_other_box_is_being_interacted_with)
  {
    was_down                = ctx->interacted_with_box_id__is_mouse_down;
    left_box_while_was_down = ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down;

    if (is_hovered && !was_down) // Mouse is up, check if we it goes down
    {
      // note: This has a bit of de sync relative to the is_hovered bool since we test if is hovered based on a different mouse pos than the one that was when the mouse went down, most of the time this shoud be fine, but i am not sure about the other times
      //       Might be nice to use mouse_pos from the prev frame or somethign like that, for now it should be fine
      B32 mouse_left_went_down = false;
      {
        OS_Event_list* events = os_get_frame_event_list();
        for (OS_Event* ev = events->first; ev; ev = ev->next)
        {
          if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_down)
          {
            mouse_left_went_down = true;
            os_consume_frame_event(ev);
          }
        }
      }

      if (mouse_left_went_down)  
      {
        // New box is interacted, so setting the state for it
        Assert(!was_down);
        Assert(!left_box_while_was_down);
        Assert(!ctx->interacted_with_box_id__is_mouse_down);
        Assert(!ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down);
        Assert(str8_match(ctx->interacted_with_box_id, Str8{}, 0));

        is_down = true;
        ctx->interacted_with_box_id__is_mouse_down = true;
        ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = false;
        ctx->interacted_with_box_id = str8_copy_alloc(ui_get_build_arena(), this_frames_box->id);
      }
    }
    else if (was_down) 
    {
      is_down = true;

      if (!is_hovered && is_down) { 
        left_box_while_was_down = true; 
        ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = true;
      }

      // todo: The events api sucks right now, but if it works, i will make a better one
      B32 mouse_left_went_up = false;
      {
        OS_Event_list* events = os_get_frame_event_list();
        for (OS_Event* ev = events->first; ev; ev = ev->next)
        {
          if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_up)
          {
            mouse_left_went_up = true;
            os_consume_frame_event(ev);
            break;
          }
        }
      }

      if (mouse_left_went_up)
      {
        Assert(was_down);
        Assert(ctx->interacted_with_box_id__is_mouse_down);

        is_down = false;
        ctx->interacted_with_box_id__is_mouse_down                      = false;
        ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = false;
        ctx->interacted_with_box_id = Str8{};
      }
    }
  }

  is_active    = str8_match(ctx->active_box_id, this_frames_box->id, 0);
  is_navigated = str8_match(ctx->navigated_box_id, this_frames_box->id, 0);

  result_actions->old_box                 = prev_frames_box;            
  result_actions->new_box                 = this_frames_box;            
  result_actions->is_hovered              = is_hovered;            
  result_actions->is_down                 = is_down;               
  result_actions->was_down                = was_down;              
  result_actions->left_box_while_was_down = left_box_while_was_down;
  result_actions->is_clicked              = was_down && !is_down && !left_box_while_was_down;
  result_actions->went_down               = !was_down && is_down;
  result_actions->went_up                 = was_down && !is_down;  
  result_actions->is_active               = is_active;
  result_actions->is_navigated            = is_navigated;

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
// - Some new stuff that is yet unstructured
//
void ui_set_active_id(Str8 id)
{
  if (id.count == 0) { return; }

  UI_Context* ctx = ui_get_context();

  // I guess this is how it is supposed to work, not sure about reset yet thought
  ctx->active_box_id    = str8_copy_alloc(ui_get_build_arena(), id);
  ctx->navigated_box_id = str8_copy_alloc(ui_get_build_arena(), id);
}

void ui_set_active_box(UI_Box* box)
{
  ui_set_active_id(box->id);
}

void ui_reset_active_id(Str8 id)
{
  UI_Context* ctx = ui_get_context();
  if (str8_match(ctx->active_box_id, id, 0)) {
    ui_reset_active();
  }
}

void ui_reset_active()
{
  UI_Context* ctx = ui_get_context();
  ctx->active_box_id = Str8{};
}

B32 ui_is_active_id(Str8 id)
{
  if (id.count == 0) { return false; }
  UI_Context* ctx = ui_get_context();
  return str8_match(ctx->active_box_id, id, 0);
}

B32 ui_is_active_box(UI_Box* box)
{
  return ui_is_active_id(box->id);
}

B32 ui_has_active() 
{
  return (ui_get_context()->active_box_id.count != 0);
}

void ui_set_b_color(UI_Box* box, V4F32 color)
{
  if (ui_box_is_zero(box)) { return; }
  box->vertex_colors[UV__00] = color;
  box->vertex_colors[UV__01] = color;
  box->vertex_colors[UV__10] = color;
  box->vertex_colors[UV__11] = color;
}

void ui_set_cursor(OS_Cursor cursor) 
{
  UI_Context* ctx = ui_get_context();
  ctx->final_cursor = cursor;
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

void ui_push_layout_axis(Axis2 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_pop_layout_axis()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }
void ui_set_next_layout_axis(Axis2 v)  { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_pop_single_usage_layout_axis() { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, layout_axis_stack, UI_Layout_axis_node) }
Axis2 ui_get_layout_axis()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, layout_axis) }

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

void ui_push_border_width(F32 v)          { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, border_width_stack, UI_Border_width_node, v) }
void ui_pop_border_width()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, border_width_stack, UI_Border_width_node) }
void ui_set_next_border_width(F32 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, border_width_stack, UI_Border_width_node, v) }
void ui_pop_single_usage_border_width()   { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, border_width_stack, UI_Border_width_node) }
F32  ui_get_border_width()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, border_width_stack, UI_Border_width_node, border_width) }

void  ui_push_border_color(V4F32 v)         { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, border_color_stack, UI_Border_color_node, v) }
void  ui_pop_border_color()                 { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, border_color_stack, UI_Border_color_node) }
void  ui_set_next_border_color(V4F32 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, border_color_stack, UI_Border_color_node, v) }
void  ui_pop_single_usage_border_color()    { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, border_color_stack, UI_Border_color_node) }
V4F32 ui_get_border_color()                 { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, border_color_stack, UI_Border_color_node, border_color) }

void ui_push_padding(F32 v)        { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, padding_stack, UI_Padding_node, v) }
void ui_pop_padding()              { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, padding_stack, UI_Padding_node) }
void ui_set_next_padding(F32 v)    { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, padding_stack, UI_Padding_node, v) }
void ui_pop_single_usage_padding() { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, padding_stack, UI_Padding_node) }
F32  ui_get_padding()              { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, padding_stack, UI_Padding_node, padding) }

void ui_push_child_gap(F32 v)          { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, child_gap_stack, UI_Child_gap_node, v) }
void ui_pop_child_gap()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, child_gap_stack, UI_Child_gap_node) }
void ui_set_next_child_gap(F32 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, child_gap_stack, UI_Child_gap_node, v) }
void ui_pop_single_usage_child_gap()   { UI_Context* ctx = ui_get_context(); _U_StyleStackPopSigngleUsage_Imp(ctx, child_gap_stack, UI_Child_gap_node) }
F32  ui_get_child_gap()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, child_gap_stack, UI_Child_gap_node, child_gap) }

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
void ui_draw_box(UI_Box* root, RangeV2F32 parent_scissor_bbox)
{
  #if DEBUG_MODE
  // if (str8_match(root->id, Str8FromC("test id"), 0)) { BP; }
  #endif

  // Custom draw func does a fully cursom draw
  if (root->custom_draw_func != 0) 
  {
    root->custom_draw_func(root); 
  }
  else 
  {
    Rect rect       = rect_from_range_v2f32(root->final_on_screen_bbox);
    RangeV2F32 bbox = root->final_on_screen_bbox;

    if (root->flags & UI_Box_flag__has_background) {
      d_draw_rect_pro(rect, root->vertex_colors[UV__00], root->vertex_colors[UV__01], root->vertex_colors[UV__10], root->vertex_colors[UV__11], root->corner_radii, root->softness); 
    }

    if (root->flags & UI_Box_flag__has_text_contents) {
      d_draw_text(root->text, root->font, rect_get_origin(rect), white()); 
    }
    
    if (root->flags & UI_Box_flag__has_borders)
    {
      d_draw_rect_inset_borders(rect, root->border_color, root->border_width, root->corner_radii, root->softness);
    }
  
    // Have to scissor ______ (THATS WHAT SHE SAID !!!)
    RangeV2F32 new_scissor_bbox = parent_scissor_bbox;
    for EachEnumRange(axis, Axis2, Axis2__x, Axis2__COUNT) { 
      if (root->flags & UI_Box_flag__clip_x<<axis) {
        // todo/note: This is error prone, since swapping range 1 and range 2 produces different results, i dont like that 
        new_scissor_bbox = intersect_range_v2f32_on_axis(new_scissor_bbox, bbox, axis);
      }
    }
    if (!range_v2f3_match(new_scissor_bbox, parent_scissor_bbox)) {
      d_push_scissor_rect(rect_from_range_v2f32(new_scissor_bbox));
    }

    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      ui_draw_box(child, new_scissor_bbox);
    }
  
    // No longer scissoring
    if (!range_v2f3_match(new_scissor_bbox, parent_scissor_bbox)) {
      d_pop_scissor_rect();
    }
  }
}

void ui_draw()
{
  // todo: Dont pass Rect here like this 
  UI_Context* ctx = ui_get_context();
  ui_draw_box(ctx->root_box, ctx->root_box->final_on_screen_bbox);
}

#endif











