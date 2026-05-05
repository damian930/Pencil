#ifndef __UI_CPP
#define __UI_CPP

#include "core/core_include.cpp"
#include "ui/ui_core.h"

// todo: Struture this file based on the structure of the .h ui file

UI_Context* _ui_g_context = 0;
UI_Box _ui_g_zero_box = {};
V2F32 _ui_g_clip_offset_stub = {};
// V2F32 _ui_g_text_measuring_stub_f(Str8 text, Font font, F32 font_size) { Assert(0, "Dude, you forgot to set a text measuing function."); return V2F32{}; }

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
// UI_Size ui_text_size()                       { return ui_size_make(UI_Size_kind__text, 0.0f, 1.0f); }         // note: value is 0.0f there cause it is actually not used by the implementation
UI_Size ui_p_of_p(F32 value, F32 strictness) { return ui_size_make(UI_Size_kind__percent_of_parent, value, strictness); }

// Just in case if i need these
UI_Size ui_px_ex(F32 value, F32 strictness) { return ui_size_make(UI_Size_kind__px, value, strictness); }
UI_Size ui_children_sum_ex(F32 strictness)  { return ui_size_make(UI_Size_kind__children_sum, 0.0f, strictness); } // note: value is 0.0f there cause it is actually not used by the implementation
// UI_Size ui_text_size_ex(F32 strictness)     { return ui_size_make(UI_Size_kind__text, 0.0f, strictness); }         // note: value is 0.0f there cause it is actually not used by the implementation

UI_Context* ui_get_context()
{
  return _ui_g_context;
}

void ui_set_context(UI_Context* context)
{
  _ui_g_context = context;
}

Arena* ui_build_arena()
{
  UI_Context* ctx = ui_get_context();
  return ctx->build_arenas[ctx->build_generation % ArrayCount(ctx->build_arenas)];
}

F32 ui_get_mouse_x()    { UI_Context* ctx = ui_get_context(); return ctx->mouse_x;  }
F32 ui_get_mouse_y()    { UI_Context* ctx = ui_get_context(); return ctx->mouse_y;  }
V2F32 ui_get_mouse() { return v2f32(ui_get_mouse_x(), ui_get_mouse_y()); }


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

  // _ui_g_context->text_measuring_fp   = _ui_g_text_measuring_stub_f; 
  _ui_g_context->root_box            = &_ui_g_zero_box;
  _ui_g_context->current_parent_box  = &_ui_g_zero_box;
  _ui_g_context->prev_frame_root_box = &_ui_g_zero_box; 
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
  Arena* arena = ui_build_arena();

  flags |= ui_get_flags();
  
  UI_Box* box = ArenaPush(arena, UI_Box);
  box->flags                   = flags;
  box->layout_axis             = ui_get_layout_axis();   
  box->semantic_size[Axis2__x] = ui_get_size_x();        
  box->semantic_size[Axis2__y] = ui_get_size_y();        
  
  box->id = str8_copy_alloc(ui_build_arena(), id_and_text);

  if (flags & UI_Box_flag__has_background)    { box->shape_style.color         = ui_get_color(); }
  // if (flags & UI_Box_flag__draw_corner_radius) { box->shape_style.corner_radius = ui_get_corner_radius(); }
  if (flags & UI_Box_flag__has_borders) {
    box->shape_style.border = ui_get_border();
  }

  // if (flags & UI_Box_flag__draw_text_contents)
  // {
  //   Str8 text = ui_get_text_part_from_str8(id_and_text);
  //   box->text_style.text       = str8_copy_alloc(ui_build_arena(), text);    
  //   box->text_style.font       = ui_get_font();       
  //   box->text_style.font_size  = ui_get_font_size();  
  //   box->text_style.text_color = ui_get_text_color(); 
  // }

  // UI_Box* this_box_prev_frame = ui_get_box_prev_frame(id_and_text);
  // if (!ui_box_is_zero(this_box_prev_frame)) 
  // {
  //   // box->clip_offset = this_box_prev_frame->clip_offset;
  // }

  DllPushBack_Name(ctx->current_parent_box, box, first_child, last_child, next_sibling, prev_sibling);
  box->parent = ui_get_parent();
  box->parent->children_count += 1;

  return box;
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

void ui_begin_build(F32 window_width, F32 window_height, F32 mouse_x, F32 mouse_y)
{
  Assert(IsMemZero(_ui_g_clip_offset_stub));
  _ui_g_clip_offset_stub = {};

  UI_Context* ctx = ui_get_context();
  
  // Resetting the prev build state
  __ui_clear_style_stacks();
  ctx->prev_frame_root_box = ctx->root_box;
  ctx->root_box = &_ui_g_zero_box;
  ctx->current_parent_box = &_ui_g_zero_box;
  arena_clear(ctx->style_stacks_arena);

  // Creating the new build state
  ctx->build_generation += 1;
  Arena* arena = ui_build_arena();
  arena_clear(arena);
  
  // Copying these since they are allocated on old build arenas
  ctx->currently_active_box_id = str8_copy_alloc(ui_build_arena(), ctx->currently_active_box_id);
  ctx->currently_interacted_with_box_id = str8_copy_alloc(ui_build_arena(), ctx->currently_interacted_with_box_id);

  __ui_push_defaults_onto_stacks();

  ui_set_next_size_x(ui_px(window_width));
  ui_set_next_size_y(ui_px(window_height));
  UI_Box* box = ui_box_make(Str8FromC("## __UI ROOT ELEMENT ID __"), 0);
  ui_push_parent(box);

  ctx->current_parent_box = box;
  ctx->root_box = box;

  ctx->mouse_x = mouse_x;
  ctx->mouse_y = mouse_y;
}

void ui_end_build()
{
  UI_Context* ctx = ui_get_context();
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
    } break;

    // case UI_Size_kind__text:
    // {
    //   UI_text_measuring_ft* text_mesure_f = ui_get_text_measuring_function();
    //   V2F32 dims = text_mesure_f(root->text_style.text, root->text_style.font, root->text_style.font_size);
    //   root->final_on_screen_size.v[axis] = dims.v[axis];
    // } break;
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

    // case UI_Size_kind__em:
    // {
    //   UI_Box* parent = root->parent;
    //   for (;;parent = parent->parent)
    //   {
    //     if (parent->text_style.font_size != 0.0f) { break; }
    //   }
    //   root->final_on_screen_size.v[axis] = parent->text_style.font_size * root->semantic_size[axis].value;
    // } break;
  }

  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_do_sizing_for_parent_dependant_box(child, axis);
  }
}

void ui_do_sizing_for_child_dependant_box(UI_Box* root, Axis2 axis)
{
  // Setting up sizes for children
 
  // if (str8_match(root->id, Str8FromC("Test id __"), 0))
  // {
  //   BreakPoint();
  // }

  // if (str8_match(root->id, Str8FromC("Floating label id inside slider"), 0))
  // {
  //   BreakPoint();
  // }

  // if (str8_match(root->id, Str8FromC("Y stack id "), 0))
  // {
  //   BreakPoint();
  // }
  
  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
    ui_do_sizing_for_child_dependant_box(child, axis);
  }
  
  // todo: Floating dont contribute to parent elements
  //       But still have to fit within

  switch (root->semantic_size[axis].kind)
  {
    default: { } break;

    case UI_Size_kind__children_sum:
    {
      for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
      {
        // note: Floating children dont attribute to the overall size of the parent
        // if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 
     
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
          // if (child->flags & UI_Box_flag__floating_x<<axis) { /*BreakPoint();*/  continue; }

          F32 child_size = child->final_on_screen_size.v[axis];
          F32 p_to_to_keep = child->semantic_size[axis].strictness;
          F32 p_to_give_out = 1.0f - p_to_to_keep;
          F32 size_to_give_out = child_size * p_to_give_out;
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
          // if (child->flags & UI_Box_flag__floating_x<<axis) { /*BreakPoint();*/ continue; } 

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
  // todo: Floating elements are not a part of the childrens tree, 
  //       what do we do about their overflow ???
  F32 available_space = root->final_on_screen_size.v[axis];
  // if (root->flags & UI_Box_flag__floating_x<<axis)
  // {
    // available_space = root->parent->final_on_screen_size.v[axis];
  // }

  // if (str8_match(root->id, Str8FromC("Y stack id "), 0) && axis == 1)
  // {
  //   BreakPoint();
  // }

  // note: Not implemented for the other axis
  if (root->layout_axis == axis)
  {
    F32 space_used_by_children = 0.0f;
    F32 total_space_children_might_give_out = 0.0f;
    for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
    {
      // if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 

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
      // if (child->flags & UI_Box_flag__floating_x<<axis) { continue; } 

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
    /*
    if (child->flags & UI_Box_flag__floating_x<<axis) 
    {
      child->final_parent_offset.v[axis] = 0.0f; // note: If floating, we leave its start where the parent starts
    }
    else*/ if (root->layout_axis == axis)
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

  if (v4f32_match(root->shape_style.color, { 0, 255, 255, 255 }))
  {
    U32 x = 0;
    // BreakPoint();
  }

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
  ui_do_sizing_for_parent_dependant_box(root, axis);
  ui_do_sizing_for_child_dependant_box(root, axis);

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

UI_Box_data ui_get_box_data_prev_frame(Str8 id)
{
  UI_Box_data box_data = {};
  UI_Box* box = ui_get_box_prev_frame(id);
  if (!ui_box_is_zero(box)) 
  { 
    box_data.on_screen_rect = box->final_on_screen_rect; 
    box_data.is_found = true; 
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

B32 ui_is_no_active()
{
  UI_Context* ctx = ui_get_context();
  B32 result = (ctx->currently_active_box_id.count == 0);
  return result; 
}

B32 ui_has_active()
{
  Str8 id = ui_get_context()->currently_active_box_id;
  B32 result = !str8_match(id, Str8{}, 0);
  return result;
}

B32 ui_is_active(Str8 box_id)
{
  UI_Context* ctx = ui_get_context();
  B32 result = str8_match(ctx->currently_active_box_id, box_id, 0); 
  return result;
}

void ui_reset_active()
{
  UI_Context* ctx = ui_get_context();
  ctx->currently_active_box_id = Str8{};
}

void ui_reset_active_match(Str8 id_to_match)
{
  UI_Context* ctx = ui_get_context();
  if (str8_match(id_to_match, ctx->currently_active_box_id, 0))
  {
    ui_reset_active();
  }
}

void ui_set_active(Str8 box_id)
{
  UI_Context* ctx = ui_get_context();
  ctx->currently_active_box_id = str8_copy_alloc(ui_build_arena(), box_id); 
}

/*
UI_Actions ui_actions_from_box(UI_Box* this_frames_box, RLI_Event_list* rli_events)
{
  // todo: I dont know how to make this be nicer in the api yet
  // todo: more than 1 update for the same box is possible in the same frame
  if (this_frames_box->id.count == 0) { return UI_Actions{}; }

  Assert(this_frames_box->has_been_updated_this_build == false); // todo: Deal with this better
  this_frames_box->has_been_updated_this_build = true;

  if (this_frames_box->id.count == 0) { return UI_Actions{}; }
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
      for (RLI_Event* e = rli_events->first; e; e = e->next)
      {
        if (e->kind == RLI_Event_kind__mouse_button && e->key_went_down && e->key == RLI_Key__mouse_left)
        { 
          // note: If nothing was down then we shoudnt have had an active box
          Assert(!was_down);
          Assert(ctx->currently_interacted_with_box_id.count == 0); 
          Assert(ctx->currently_interacted_with_box__is_down == false);
          Assert(ctx->currently_interacted_with_box__left_box_while_was_down == false);
          
          RLI_consume_event(e);
          is_down = true;
          ctx->currently_interacted_with_box_id = str8_copy_alloc(ui_build_arena(), this_frames_box->id);
          ctx->currently_interacted_with_box__is_down = true;
          break;
        }
      }
    }
    else if (was_down)
    {
      is_down = was_down;

      if (!is_hovered && is_down) { 
        left_box_while_was_down = true; 
        ctx->currently_interacted_with_box__left_box_while_was_down = true;
      }
  
      for (RLI_Event* e = rli_events->first; e; e = e->next)
      {
        if (e->kind == RLI_Event_kind__mouse_button && e->key_came_up && e->key == RLI_Key__mouse_left)
        { 
          Assert(ctx->currently_interacted_with_box_id.count != 0); // note: If nothing was down then we shoudnt have had an active box
          RLI_consume_event(e);
          is_down = false;

          // Resetting retained stuff
          ctx->currently_interacted_with_box_id = Str8{};
          ctx->currently_interacted_with_box__is_down = false;
          ctx->currently_interacted_with_box__left_box_while_was_down = false;
          
          break;
        }
      }
    }
  }

  UI_Actions result_actions = {};
  result_actions.is_hovered = is_hovered;
  result_actions.is_down    = is_down;
  result_actions.was_down   = was_down;
  result_actions.left_box_while_was_down = left_box_while_was_down;
  result_actions.is_clicked = was_down && !is_down && !left_box_while_was_down;
  result_actions.wheel_move = mouse_wheel_move;

  // todo: not sure about these for now
  {
    if (is_hovered)
    {
      for (RLI_Event* e = rli_events->first; e; e = e->next)
      {
        if (e->kind == RLI_Event_kind__mouse_wheel)
        {
          result_actions.wheel_move = e->mouse_wheel_move;
          RLI_consume_event(e);
          break;
        }
      }
    }

    if (is_hovered || ui_is_active(this_frames_box->id))
    {
      for (RLI_Event* ev = rli_events->first; ev; ev = ev->next)
      {
        if (ev->kind == RLI_Event_kind__keyboard_button && ev->key == RLI_Key__enter && ev->key_went_down)
        {
          RLI_consume_event(ev);
          result_actions.enter_got_pressed = true; 
          break;
        }
      }
    }
  }

  return result_actions;
}

UI_Actions ui_actions_from_id(Str8 id, RLI_Event_list* rli_events)
{
  UI_Actions actions = {};
  UI_Box* box = ui_get_box_prev_frame(id);
  if (!ui_box_is_zero(box)) { actions = ui_actions_from_box(box, rli_events); }
  return actions;
}
*/

///////////////////////////////////////////////////////////
// - Style stacks
//
#define _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val)    \
  node_type* node = ArenaPush(ctx_p->style_stacks_arena, node_type);            \
  node->v = val;                                                                 \
  StackPush(&ctx_p->stack_name_inside_ctx, node);                                \
  ctx_p->stack_name_inside_ctx.count += 1;

#define _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type)  \
  StackPop(&ctx_p->stack_name_inside_ctx);                               \
  ctx_p->stack_name_inside_ctx.count -= 1;                               \
  ctx_p->stack_name_inside_ctx.pop_after_first_use = false;

#define _UI_StyleStackAutoPop_Impl(ctx_p, stack_name_inside_ctx, node_type)  \
  if (ctx_p->stack_name_inside_ctx.pop_after_first_use) {                    \
    _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type);         \
  }

#define _UI_StyleStackPeek_Impl(ctx_p, stack_name_inside_ctx, node_type) \
  return ctx_p->stack_name_inside_ctx.first->v;

// todo: remove auto from here
#define _UI_StyleStackGet_Impl(ctx_p, stack_name_inside_ctx, node_type)   \
  auto result = ctx_p->stack_name_inside_ctx.first->v;              \
  _UI_StyleStackAutoPop_Impl(ctx_p, stack_name_inside_ctx, node_type);    \
  return result;

#define _UI_StyleStackSetNext_Impl(ctx_p, stack_name_inside_ctx, node_type, val) \
  _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val);         \
  ctx_p->stack_name_inside_ctx.pop_after_first_use = true;

///////////////////////////////////////////////////////////
// - Style stack operations for default settings
//
void ui_push_flags(UI_Box_flags v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, flags_stack, UI_Box_flags_node, v) }
void ui_set_next_flags(UI_Box_flags v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, flags_stack, UI_Box_flags_node, v) }
void ui_pop_flags()                    { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, flags_stack, UI_Box_flags_node) }
void ui_auto_pop_flags_stack()         { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, flags_stack, UI_Box_flags_node) }
UI_Box_flags ui_peek_flags()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, flags_stack, UI_Box_flags_node) }
UI_Box_flags ui_get_flags()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, flags_stack, UI_Box_flags_node) }
void ui_add_flags(UI_Box_flags flags) 
{
  UI_Context* ctx = ui_get_context();
  ctx->flags_stack.first->v |= flags;
}
void ui_add_flags_to_next(UI_Box_flags flags) 
{
  UI_Context* ctx = ui_get_context();
  if (!ctx->flags_stack.pop_after_first_use)
  {
    ui_set_next_flags(0);
  }
  ctx->flags_stack.first->v |= flags;
}

void ui_push_layout_axis(Axis2 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_set_next_layout_axis(Axis2 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, layout_axis_stack, UI_Layout_axis_node, v) }
void ui_pop_layout_axis()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }
void ui_auto_pop_layout_axis_stack()  { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }
Axis2 ui_peek_layout_axis()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }
Axis2 ui_get_layout_axis()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, layout_axis_stack, UI_Layout_axis_node) }

void ui_push_size_x(UI_Size v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node, v) }
void ui_set_next_size_x(UI_Size v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node, v) }
void ui_pop_size_x()               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node) }
void ui_auto_pop_size_x_stack()    { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node) }
UI_Size ui_peek_size_x()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node) }
UI_Size ui_get_size_x()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, semantic_size_x_stack, UI_Semantic_size_node) }

void ui_push_size_y(UI_Size v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node, v) }
void ui_set_next_size_y(UI_Size v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node, v) }
void ui_pop_size_y()               { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node) }
void ui_auto_pop_size_y_stack()    { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node) }
UI_Size ui_peek_size_y()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node) }
UI_Size ui_get_size_y()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, semantic_size_y_stack, UI_Semantic_size_node) }

// todo: Setters for size x and y at the same time

// -

void ui_push_color_no_flag(V4F32 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, color_stack, UI_Color_node, v) }
void ui_set_next_color_no_flag(V4F32 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, color_stack, UI_Color_node, v) }
void ui_pop_color()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, color_stack, UI_Color_node) }
void ui_auto_pop_color_stack()     { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, color_stack, UI_Color_node) }
V4F32 ui_peek_color()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, color_stack, UI_Color_node) }
V4F32 ui_get_color()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, color_stack, UI_Color_node) }
void ui_push_color(V4F32 v) { ui_push_color_no_flag(v); ui_add_flags(UI_Box_flag__has_background); }
void ui_set_next_b_color(V4F32 v) { ui_set_next_color_no_flag(v); ui_add_flags_to_next(UI_Box_flag__has_background); }

// void ui_push_corner_radius_no_flag(F32 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, corner_radius_stack, UI_Corner_radius_node, v) }
// void ui_set_next_corner_radius_no_flag(F32 v)  { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, corner_radius_stack, UI_Corner_radius_node, v) }
// void ui_pop_corner_radius()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, corner_radius_stack, UI_Corner_radius_node) }
// void ui_auto_pop_corner_radius_stack() { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, corner_radius_stack, UI_Corner_radius_node) }
// F32 ui_peek_corner_radius()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, corner_radius_stack, UI_Corner_radius_node) }
// F32 ui_get_corner_radius()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, corner_radius_stack, UI_Corner_radius_node) }
// void ui_push_corner_radius(F32 v) { ui_push_corner_radius_no_flag(v); ui_add_flags(UI_Box_flag__draw_corner_radius); }
// void ui_set_next_corner_radius(F32 v) { ui_set_next_corner_radius_no_flag(v); ui_add_flags_to_next(UI_Box_flag__draw_corner_radius); }

void ui_push_border_no_flag(F32 width, V4F32 color)     { UI_Context* ctx = ui_get_context(); UI_Border_style v = {}; v.width = width; v.color = color; _UI_StyleStackPush_Impl(ctx, border_style_stack, UI_Border_style_node, v) }
void ui_set_next_border_no_flag(F32 width, V4F32 color) { UI_Context* ctx = ui_get_context(); UI_Border_style v = {}; v.width = width; v.color = color; _UI_StyleStackSetNext_Impl(ctx, border_style_stack, UI_Border_style_node, v) }
void ui_pop_border()                                    { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, border_style_stack, UI_Border_style_node) }
void ui_auto_pop_border_stack()                         { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, border_style_stack, UI_Border_style_node) }
UI_Border_style ui_peek_border()                        { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, border_style_stack, UI_Border_style_node) }
UI_Border_style ui_get_border()                         { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, border_style_stack, UI_Border_style_node) }
void ui_push_border(F32 width, V4F32 color)             { ui_push_border_no_flag(width, color); ui_add_flags(UI_Box_flag__has_borders); }
void ui_set_next_border(F32 width, V4F32 color)         { ui_set_next_border_no_flag(width, color); ui_add_flags_to_next(UI_Box_flag__has_borders); }

// --

// void ui_push_text_color(V4F32 v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, text_color_stack, UI_Text_color_node, v) }
// void ui_set_next_text_color(V4F32 v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, text_color_stack, UI_Text_color_node, v) }
// void ui_pop_text_color()                { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, text_color_stack, UI_Text_color_node) }
// void ui_auto_pop_text_color_stack()     { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, text_color_stack, UI_Text_color_node) }
// V4F32 ui_peek_text_color()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, text_color_stack, UI_Text_color_node) }
// V4F32 ui_get_text_color()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, text_color_stack, UI_Text_color_node) }

// void ui_push_font(Font v)     { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, text_font_stack, UI_Text_font_node, v) }
// void ui_set_next_font(Font v) { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, text_font_stack, UI_Text_font_node, v) }
// void ui_pop_font()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, text_font_stack, UI_Text_font_node) }
// void ui_auto_pop_font_stack() { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, text_font_stack, UI_Text_font_node) }
// Font ui_peek_font()           { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, text_font_stack, UI_Text_font_node) }
// Font ui_get_font()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, text_font_stack, UI_Text_font_node) }

// void ui_push_font_size(F32 v)      { UI_Context* ctx = ui_get_context(); _UI_StyleStackPush_Impl(ctx, text_font_size_stack, UI_Text_font_size_node, v) }
// void ui_set_next_font_size(F32 v)  { UI_Context* ctx = ui_get_context(); _UI_StyleStackSetNext_Impl(ctx, text_font_size_stack, UI_Text_font_size_node, v) }
// void ui_pop_font_size()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPop_Impl(ctx, text_font_size_stack, UI_Text_font_size_node) }
// void ui_auto_pop_font_size_stack() { UI_Context* ctx = ui_get_context(); _UI_StyleStackAutoPop_Impl(ctx, text_font_size_stack, UI_Text_font_size_node) }
// F32 ui_peek_font_size()            { UI_Context* ctx = ui_get_context(); _UI_StyleStackPeek_Impl(ctx, text_font_size_stack, UI_Text_font_size_node) }
// F32 ui_get_font_size()             { UI_Context* ctx = ui_get_context(); _UI_StyleStackGet_Impl(ctx, text_font_size_stack, UI_Text_font_size_node) }

// --

// void ui_set_next_id(Str8 id)
// {
//   UI_Context* ctx = ui_get_context();
//   UI_ID_node* node = ArenaPush(ui_build_arena(), UI_ID_node);
//   node->id = str8_copy_alloc(ui_build_arena(), id); // todo: Check is allocation in the middle of the arena
//   StackPush(&ctx->id_stack, node);
//   ctx->id_stack.count += 1;
//   ctx->id_stack.pop_after_first_use = true;
// }

// Str8 ui_get_next_id() // note: This id might be invalid next build, so be carefull
// {
//   Str8 id = {};
//   UI_Context* ctx = ui_get_context();
//   UI_ID_stack* stack = &ctx->id_stack;
//   Assert(stack->count > 0);
//   if (stack->count > 0)
//   {
//     id = stack->first->id;
//     if (stack->count > 1)
//     {
//       StackPop(stack);
//       stack->count -= 1;
//       stack->pop_after_first_use = false;
//     }
//   }
//   return id;
// }

// todo: What happends if i pop all the values and then try to get them, if fails, then we shoud protect agains the full pop case

// ========

///////////////////////////////////////////////////////////
// - Draw stuff for the ui for the ui
//
// note: This is a new, custom drawn thing for the ui, work in progress right now
void ui_draw_command_from_ui_root(Arena* arena, UI_Box* root, UI_Draw_command_list* command_list)
{
  Assert(!ui_box_is_zero(root));

  U64 _arena_pos_before_allocating_draw_command = ArenaCurrentAddressU64(arena);
  UI_Draw_command* command                     = ArenaPush(arena, UI_Draw_command);
  U64 _arena_pos_after_allocating_draw_command = ArenaCurrentAddressU64(arena);
  
  B32 shoud_make_command = false;
  Rect rect = root->final_on_screen_rect;

  if (root->flags & UI_Box_flag__has_background)
  {
    shoud_make_command = true;
    command->rect = rect;
    command->rect_color = root->shape_style.color;
  }
  if (root->flags & UI_Box_flag__has_borders)
  {
    shoud_make_command = true;
    command->rect = rect;
    command->border_width = root->shape_style.border.width;
    command->border_color = root->shape_style.border.color;
  }

  // Testing the invariant
  U64 _arena_pos_after_setting_the_command_data = ArenaCurrentAddressU64(arena);
  Assert(_arena_pos_after_setting_the_command_data - sizeof(UI_Draw_command) == _arena_pos_before_allocating_draw_command);

  if (shoud_make_command) {
    DllPushBack(command_list, command);
    command_list->count += 1;
  } else {
    ArenaPopType(arena, UI_Draw_command);
  }

  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    ui_draw_command_from_ui_root(arena, child, command_list);
  }

}

/* note: This is an old raylib version of this thing
void ui_draw_box(UI_Box* root, F32 x_clip_offset, F32 y_clip_offset)
{
  // todo: When having corner radius, dont draw the stuff on the outside of the corners for the box, since that would make no sense
  // todo: There is a bug in how we end scissor mode. Sometimes we dont end it.
  
  Rect rect = root->final_on_screen_rect;
  
  if (root->flags & UI_Box_flag__floating_x) { x_clip_offset = 0.0f; }
  if (root->flags & UI_Box_flag__floating_y) { y_clip_offset = 0.0f; }

  rect.x += x_clip_offset;
  rect.y += y_clip_offset;

  if (root->flags & UI_Box_flag__draw_background)
  {
    DrawRectangleRounded(raylib_rect, root->shape_style.corner_radius, 0, RAYLIB_COLOR_FROM_VEC(root->shape_style.color));
  }

  // todo/note: DrawRectangleRoundedLinesEx draws the line on the outside of the rect and not within
  if (root->flags & UI_Box_flag__draw_borders)
  {
    // note: Might just draw a rect behind the rect which has borders
    // DrawRectangleV();
    // DrawRing();
    DrawRectangleRoundedLinesEx(raylib_rect, root->shape_style.corner_radius, 0, root->shape_style.border.width, RAYLIB_COLOR_FROM_VEC(root->shape_style.border.color));
  }

  if (root->flags & UI_Box_flag__draw_text_contents)
  {
    Scratch scratch = get_scratch(0, 0);
    Str8 text_nt = str8_copy_alloc(scratch.arena, root->text_style.text);
    DrawTextEx(root->text_style.font, (char*)text_nt.data, Vector2{raylib_rect.x, raylib_rect.y}, (F32)root->text_style.font_size, 0, RAYLIB_COLOR_FROM_VEC(root->text_style.text_color));
    end_scratch(&scratch);
  }

  if (root->texture_to_draw.id != 0)
  {
    Texture2D texture = root->texture_to_draw;
    Rectangle source_rect = { 0.0f, 0.0f, (F32)texture.width, (F32)texture.height };
    DrawTexturePro(root->texture_to_draw, source_rect, raylib_rect, Vector2{}, 0.0f, WHITE); 
  }
  
  // todo: Scissor state might be broken if 1 scissor is actiev and another begins, so check for that
  //       it might happen when there is a parent with overflowed child and some it its children 
  //       have overflow as well anywhere deeper in that subtree.

  // This is here to not draw overflow, if need to draw it at some point, will make this a flag
  if (root->flags & UI_Box_flag__dont_draw_overflow)
  {
    BeginScissorMode((int)raylib_rect.x, (int)raylib_rect.y, (int)raylib_rect.width, (int)raylib_rect.height);
  }
  else {
    B32 is_on_x = root->flags & UI_Box_flag__dont_draw_overflow_x;
    B32 is_on_y = root->flags & UI_Box_flag__dont_draw_overflow_y;
    if (XOR(is_on_x, is_on_y)) {
      // note: I did not bother implement this. 
      //       To implement this just do something like scissor on on x and the whole ui size on y (screen y)
      InvalidCodePath();
    }
  }

  x_clip_offset += root->clip_offset.v[Axis2__x];
  y_clip_offset += root->clip_offset.v[Axis2__y];

  for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
  {
    if (child->flags & UI_Box_flag__floating_x) {
      x_clip_offset -= root->clip_offset.v[Axis2__x];    
    } 
    if (child->flags & UI_Box_flag__floating_y) {
      y_clip_offset -= root->clip_offset.v[Axis2__y];    
    }

    ui_draw_box(child, x_clip_offset, y_clip_offset);

    if (child->flags & UI_Box_flag__floating_x) {
      x_clip_offset += root->clip_offset.v[Axis2__x];    
    } 
    if (child->flags & UI_Box_flag__floating_y) {
      y_clip_offset += root->clip_offset.v[Axis2__y];    
    }
  }

  // This is here to not draw overflow, if need to draw it at some point, will make this a flag
  if (root->flags & UI_Box_flag__dont_draw_overflow)
  {
    EndScissorMode();
  }

  x_clip_offset -= root->clip_offset.v[Axis2__x];
  y_clip_offset -= root->clip_offset.v[Axis2__y];
}
*/

// void ui_draw()
// {
//   UI_Context* ctx = ui_get_context();
//   ui_draw_box(ctx->root_box, 0.0f, 0.0f);
// }



#endif











