#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/os_core.h"
#include "os/os_core.cpp"
// the win32 specific code is linked in and cannot be included, since it includes windows.h. Raylib also included windows.h and is used for ui. So we link.

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"
#include "ui/ui_widgets.h"
#include "ui/ui_widgets.cpp"

// todo: This whole shinanigans with raylib and windows get fixed if we just dont use raylib
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "third_party/raylib/raylib.lib")
#include "third_party/raylib/raymath.h"

#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define Rectangle Win32Rectangle
#include "windows.h"
#undef CloseWindow
#undef ShowCursor
#undef Rectangle

V2 ui_text_measure_func(Str8 text, Font font, F32 font_size)
{ 
  Scratch scratch = get_scratch(0, 0);
  Str8 text_nt = str8_copy_alloc(scratch.arena, text);
  Vector2 rl_vec = MeasureTextEx(font, (char*)text_nt.data, font_size, 0);
  V2 result = { rl_vec.x, rl_vec.y };
  end_scratch(&scratch);
  return result;
}

// note: This is for hooks, which i dont yet use
// #include "dwmapi.h"
// #pragma comment(lib, "dwmapi.lib")

// todo: Add a way to have multiple draw records and go back between each one
// todo: Since records are infinite, maku sure you dont run out of space 


/*
struct Node_with_points {
  V2 points[100];
  U16 points_count;
  Node_with_points* next; // This is shared with the free list 
  Node_with_points* prev; 

  // Free list part
  // B32 _is_active;
};
StaticAssert(ArrayCount(Node_with_points::points) <= u16_max);
*/

#define IMAGE_PART_DATA_WIDTH  200
#define IMAGE_PART_DATA_HEIGHT 200
struct Image_part {
  V2U64 offset;
  U64 width;
  U64 height;
  U32 pixels[IMAGE_PART_DATA_WIDTH * IMAGE_PART_DATA_HEIGHT];

  Image_part* next; 
  Image_part* prev;
};

struct Draw_record {
  U64 pen_size;
  // Node_with_points* first_node_with_points;
  // Node_with_points* last_node_with_points;
  // U64 nodes_count;

  RangeV2U64 affected_2d_range; // note: These is calculated mid drawing for quick access after we done drawing

  Image_part* first_image_part;
  Image_part* last_image_part;
  U64 image_parts_count;

  Draw_record* prev;
  
  // Free list part
  Draw_record* _next_free;
  // B32 _is_active;
};

struct Image_data {
  U32* pixels; // R8_G8_B8_A8 
  U64 width;
  U64 height;
};  

struct G_state {
  Arena* arena;
  Arena* frame_arena;
  U64 pen_size;
  V4 pen_color;

  Texture draw_texture;
  Image_data draw_texture_after_the_last_draw;  

  // Pool of draw_records  
  Arena* arena_for_draw_records;
  Draw_record* first_draw_record;
  U64 allocated_draw_records_count;
  Draw_record* first_free_draw_record;

  /*
  // Pool of nodes_with_points
  Arena* arena_for_nodes_with_points;
  Node_with_points* first_node_with_points;
  U64 allocated_nodes_with_points_count;
  Node_with_points* first_free_node_with_points;
  */

  // Pool of Image parts
  Arena* arena_for_image_parts;
  Image_part* first_image_part;
  U64 allocated_image_parts_count;
  Image_part* first_free_image_part;

  // Stuff for while drawing
  B32 is_mid_drawing;
  Draw_record* current_draw_record;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;

  // Misc
  Font font_texture_for_ui;
};

/*
Node_with_points* get_available_node_of_points_from_pool(G_state* G)
{
  Node_with_points* node = G->first_free_node_with_points;
  if (node)
  {
    G->first_free_node_with_points = G->first_free_node_with_points->next;
    *node = Node_with_points{};
  }
  else
  {
    node = ArenaPush(G->arena_for_nodes_with_points, Node_with_points);
    G->allocated_nodes_with_points_count += 1;
  }
  // node->_is_active = true;
  return node;
}
*/

Draw_record* get_available_draw_record_from_pool(G_state* G)
{
  Draw_record* draw_record = G->first_free_draw_record;
  if (draw_record)
  {
    G->first_free_draw_record = G->first_free_draw_record->_next_free;
    *draw_record = Draw_record{};
  }
  else 
  {
    draw_record = ArenaPush(G->arena_for_draw_records, Draw_record);
    G->allocated_draw_records_count += 1;
  }
  // draw_record->_is_active = true;
  return draw_record;
}

Image_part* get_available_image_part_from_pool(G_state* G)
{
  Image_part* part = G->first_free_image_part;
  if (part)
  {
    G->first_free_image_part = G->first_free_image_part->next;
  }
  else
  {
    part = ArenaPush(G->arena_for_image_parts, Image_part);
    G->allocated_image_parts_count += 1;
  }
  *part = Image_part{};
  return part;
}

U32* image_part_get_px(Image_part* part, U64 x, U64 y)
{
  Assert(x < IMAGE_PART_DATA_WIDTH); Assert(y < IMAGE_PART_DATA_HEIGHT); 
  U32* px = part->pixels + (y * IMAGE_PART_DATA_WIDTH + x);
  return px;
}

void update_pencil(G_state* G, B32 is_ui_capturing_mouse)
{
  Assert(G->first_draw_record != 0);
  // Assert(G->first_node_with_points != 0);
  
  // Handling signals
  {
    if (G->signal_new_pen_size)
    {
      Assert(G->is_mid_drawing == false); // Just making sure
      if (!G->is_mid_drawing)
      {
        G->pen_size = G->new_pen_size;
        G->signal_new_pen_size = false;
        G->new_pen_size = 0;
      }
    }
  }

  // User is no longer holding the mouse, have to end drawing mode 
  if (G->is_mid_drawing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
  {
    // Now that we are done drawing, we have to get the affected rect 
    // in the current draw texture and store it in the current record.
    // This will allow for later removing of the record, since we will know what to put instead of it. 
    Assert(G->current_draw_record != 0);
    Assert(range_v2u64_is_valid(G->current_draw_record->affected_2d_range)); 

    G->is_mid_drawing = false;

    RangeV2U64 affected_range = G->current_draw_record->affected_2d_range;
    
    const U64 count_affectd_px_on_x = affected_range.max.x - affected_range.min.x;
    const U64 count_affectd_px_on_y = affected_range.max.y - affected_range.min.y;
    
    U64 n_parts_we_need_on_x = (U64)(count_affectd_px_on_x / IMAGE_PART_DATA_WIDTH) + 1;
    U64 n_parts_we_need_on_y = (U64)(count_affectd_px_on_y / IMAGE_PART_DATA_HEIGHT) + 1;

    Image_part** image_parts_arr = ArenaPushArr(G->frame_arena, Image_part*, n_parts_we_need_on_x * n_parts_we_need_on_y);

    // Allocating parts to cover the affected area
    U64 count_affectd_px_on_x_mutable = count_affectd_px_on_x;
    U64 count_affectd_px_on_y_mutable = count_affectd_px_on_y;
    for (U64 part_index_y = 0; part_index_y < n_parts_we_need_on_y; part_index_y += 1)
    {
      for (U64 part_index_x = 0; part_index_x < n_parts_we_need_on_x; part_index_x += 1)
      {
        Image_part* new_part = get_available_image_part_from_pool(G);

        image_parts_arr[part_index_y * n_parts_we_need_on_x + part_index_x] = new_part;

        // Next/Prev 
        DllPushBack_Name(G->current_draw_record, new_part, first_image_part, last_image_part, next, prev);
        G->current_draw_record->image_parts_count += 1;

        // Offset 
        new_part->offset = {};
        new_part->offset.x = part_index_x * IMAGE_PART_DATA_WIDTH + affected_range.min.x;
        new_part->offset.y = part_index_y * IMAGE_PART_DATA_HEIGHT + affected_range.min.y; 
        
        // Width
        if (count_affectd_px_on_x_mutable > IMAGE_PART_DATA_WIDTH) {
          new_part->width = IMAGE_PART_DATA_WIDTH;
          count_affectd_px_on_x_mutable -= IMAGE_PART_DATA_WIDTH;
        } else {
          new_part->width = count_affectd_px_on_x_mutable;
          count_affectd_px_on_x_mutable -= count_affectd_px_on_x_mutable; 
        }

        // Height
        if (count_affectd_px_on_y_mutable > IMAGE_PART_DATA_HEIGHT) {
          new_part->height = IMAGE_PART_DATA_HEIGHT;
          count_affectd_px_on_y_mutable -= IMAGE_PART_DATA_HEIGHT;
        } else {
          new_part->height = count_affectd_px_on_y_mutable;
          count_affectd_px_on_y_mutable -= count_affectd_px_on_y_mutable; 
        }

        // note: Data will be set in the next routine
      }
    }
    Assert(count_affectd_px_on_x_mutable == 0);
    Assert(count_affectd_px_on_y_mutable == 0);

    // Iterating over draw image pixels and storing them into image parts 
    for (
      U64 draw_texture_affected_px_index_x = affected_range.min.x; 
      draw_texture_affected_px_index_x < affected_range.max.x; 
      draw_texture_affected_px_index_x += 1
    ) {
      for (
        U64 draw_texture_affected_px_index_y = affected_range.min.y; 
        draw_texture_affected_px_index_y < affected_range.max.y; 
        draw_texture_affected_px_index_y += 1
      ) {
        U64 px_index_x_in_affected_range = (draw_texture_affected_px_index_x - affected_range.min.x);
        U64 px_index_y_in_affected_range = (draw_texture_affected_px_index_y - affected_range.min.y);

        U64 part_index_x = (U64)(px_index_x_in_affected_range / IMAGE_PART_DATA_WIDTH);
        U64 part_index_y = (U64)(px_index_y_in_affected_range / IMAGE_PART_DATA_HEIGHT);

        Image_part* part = image_parts_arr[part_index_y * n_parts_we_need_on_x + part_index_x];

        U64 px_index_x_in_part = px_index_x_in_affected_range - (part_index_x * IMAGE_PART_DATA_WIDTH);
        U64 px_index_y_in_part = px_index_y_in_affected_range - (part_index_y * IMAGE_PART_DATA_HEIGHT);

        U32* part_px = image_part_get_px(part, px_index_x_in_part, px_index_y_in_part);

        U64 px_index_inside_draw_texture = (G->draw_texture.width * draw_texture_affected_px_index_y + draw_texture_affected_px_index_x);
        *part_px = G->draw_texture_after_the_last_draw.pixels[px_index_inside_draw_texture];
      }
    }
  }

  // Drawing or about to start drawing
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
  {
    // Starting to draw a new record
    if (!G->is_mid_drawing && !is_ui_capturing_mouse) 
    { 
      G->is_mid_drawing = true;
      
      Draw_record* new_draw_record = get_available_draw_record_from_pool(G);
      new_draw_record->prev = G->current_draw_record; 

      G->current_draw_record = new_draw_record;
      G->current_draw_record->pen_size = G->pen_size;

      /*
      Node_with_points* new_node = get_available_node_of_points_from_pool(G); // These nodes are just lists, so there is nothing to set up for them to work after getting the pool memory for them
      G->current_draw_record->first_node_with_points = new_node; 
      G->current_draw_record->last_node_with_points  = new_node; 
      G->current_draw_record->nodes_count = 0; 
      */

      G->current_draw_record->affected_2d_range.min = v2u64(G->draw_texture.width, G->draw_texture.height);
      G->current_draw_record->affected_2d_range.max = v2u64(0, 0);
    }

    if (G->is_mid_drawing) 
    {
      // todo: Use a u64 vector to not have to cast each time you use this 
      Vector2 mouse_pos = GetMousePosition();
      clamp_f32_inplace(&mouse_pos.x, 0.0f, (F32)GetScreenWidth());
      clamp_f32_inplace(&mouse_pos.y, 0.0f, (F32)GetScreenHeight());
      
      // note: Count of a linked to node cant be zero
      Assert(G->pen_size != 0 && G->pen_size % 2 == 1); // Just making sure that it is an odd value
      // Assert(G->current_draw_record && G->current_draw_record->_is_active);
      // Assert(G->current_draw_record->first_node_with_points != 0);
      // Assert(G->current_draw_record->last_node_with_points != 0);
      
      // Node_with_points* last_node = G->current_draw_record->last_node_with_points;
      // B32 prev_point_exists = false;
      // V2 prev_point = {};
      
      // Adding a new node up front if the last one is full
      // if (last_node->points_count == ArrayCount(last_node->points))
      // {
        // Node_with_points* new_node = get_available_node_of_points_from_pool(G);
        // DllPushBack_Name(G->current_draw_record, new_node, first_node_with_points, last_node_with_points, next, prev);
        // G->current_draw_record->nodes_count += 1;
        // last_node = new_node;
      // }
      
      // Checking if the last point exists 
      // if (last_node->points_count == 0 && last_node->prev != 0)
      // {
        // Assert(last_node->prev->points_count == ArrayCount(last_node->prev->points), "Why the fuck do you have a 0 filled node that has an unfilled prev node ???"); 
        // prev_point = last_node->prev->points[last_node->prev->points_count - 1];
        // prev_point_exists = true;
      // }
      // else 
      // {
        // prev_point = last_node->points[last_node->points_count - 1];
        // prev_point_exists = true;
      // }
  
      // Storing the new points
      // B32 shoud_draw_point = false;
      // if (!prev_point_exists || mouse_pos.x != prev_point.x || mouse_pos.y != prev_point.y)
      // {
        // Assert(last_node->points_count != ArrayCount(last_node->points)); // Just checking
        // V2* point = last_node->points + (last_node->points_count++);
        // point->x = mouse_pos.x;
        // point->y = mouse_pos.y;
        // shoud_draw_point = true;
      // }
  
      // Drawing
      {
        Assert(G->pen_size % 2 == 1); // Hard to draw pen_size, which is the diameter when we press on the middle px, but there is not middle pixel, since the diameter is an even value
  
        // Upating min/max points
        {
          U64 pen_radius_offset = (U64)(G->pen_size / 2);
          
          // Min
          {
            V2U64* min_point = &G->current_draw_record->affected_2d_range.min;
            V2U64 test_min_point = {};
            if ((U64)mouse_pos.x > pen_radius_offset) { test_min_point.x = (U64)mouse_pos.x - pen_radius_offset; } // Making sure we dont overflow the U64 and loop around
            if ((U64)mouse_pos.y > pen_radius_offset) { test_min_point.y = (U64)mouse_pos.y - pen_radius_offset; } // Making sure we dont overflow the U64 and loop around
            min_point->x = Min(min_point->x, test_min_point.x);
            min_point->y = Min(min_point->y, test_min_point.y);
          }

          // Max
          { 
            V2U64* max_point = &G->current_draw_record->affected_2d_range.max;
            V2U64 test_max_point = v2u64((U64)mouse_pos.x + pen_radius_offset + 1, (U64)mouse_pos.y + pen_radius_offset + 1); // + 1 to x an y cause the standard is to have ranges be inclusive on min and exclusive on y, so i make it exclusive
            test_max_point.x = Min(test_max_point.x, (U64)GetScreenWidth());
            test_max_point.y = Min(test_max_point.y, (U64)GetScreenHeight());
            Assert(test_max_point.x <= G->draw_texture.width);            
            Assert(test_max_point.y <= G->draw_texture.height);            
            max_point->x = Max(max_point->x , test_max_point.x);
            max_point->y = Max(max_point->y , test_max_point.y);
          }
        }

        // Updating the texture
        Temp_arena temp = temp_arena_begin(G->frame_arena);
        U32* pixels_to_update = ArenaPushArr(temp.arena, U32, G->pen_size * G->pen_size);
        for (U64 col_index = 0; col_index < G->pen_size; col_index += 1)
        {
          for (U64 row_index = 0; row_index < G->pen_size; row_index += 1)
          {
            U32* px = pixels_to_update + (G->pen_size * row_index + col_index);
            ((U8*)(px))[0] = (U8)G->pen_color.r;
            ((U8*)(px))[1] = (U8)G->pen_color.g;
            ((U8*)(px))[2] = (U8)G->pen_color.b;
            ((U8*)(px))[3] = (U8)G->pen_color.a; 
          }
        }
        U64 pixels_on_each_side_to_the_middle_pixel = (U64)(G->pen_size / 2);
        Rectangle affected_rect = {};
        affected_rect.x      = mouse_pos.x - pixels_on_each_side_to_the_middle_pixel;
        affected_rect.y      = mouse_pos.y - pixels_on_each_side_to_the_middle_pixel;
        affected_rect.width  = (F32)G->pen_size;
        affected_rect.height = (F32)G->pen_size;
        UpdateTextureRec(G->draw_texture, affected_rect, pixels_to_update);
        temp_arena_end(&temp);
      }
    }
  } 
  else if (IsKeyPressed(KEY_BACKSPACE))
  {
    if (G->current_draw_record)
    {
      Assert(G->current_draw_record->pen_size % 2 == 1); // Hard to draw pen_size, which is the diameter when we press on the middle px, but there is not middle pixel, since the diameter is an even value
    
      // todo: Removing the record from the draw texture by drawing stored parts back onto the texture

      // Removing the record from draw_texure
      for (Image_part* part = G->current_draw_record->first_image_part; part; part = part->next)
      {
        Rectangle rect = { (F32)part->offset.x, (F32)part->offset.y, (F32)part->width, (F32)part->height };
        UpdateTextureRec(G->draw_texture, rect, part->pixels);
      }
      // for (Node_with_points* node = G->current_draw_record->first_node_with_points; node; node = node->next)
      // {
      //   for (U64 p_index = 0; p_index < node->points_count; p_index += 1)
      //   {
      //     U64 pen_size = G->current_draw_record->pen_size;
      //     Temp_arena temp = temp_arena_begin(G->frame_arena);
      //     U32* pixels_to_update = ArenaPushArr(temp.arena, U32, pen_size * pen_size);
      //     for (U64 col_index = 0; col_index < pen_size; col_index += 1)
      //     {
      //       for (U64 row_index = 0; row_index < pen_size; row_index += 1)
      //       {
      //         U32* px = pixels_to_update + (pen_size * row_index + col_index);
      //         ((U8*)(px))[0] = 0; 
      //         ((U8*)(px))[1] = 0;  
      //         ((U8*)(px))[2] = 0; 
      //         ((U8*)(px))[3] = 0; 
      //       }
      //     }
      //     U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);
      //     Rectangle affected_rect = {};
      //     affected_rect.x      = node->points[p_index].x - pixels_on_each_side_to_the_middle_pixel;
      //     affected_rect.y      = node->points[p_index].y - pixels_on_each_side_to_the_middle_pixel;
      //     affected_rect.width  = (F32)pen_size;
      //     affected_rect.height = (F32)pen_size;
      //     UpdateTextureRec(G->draw_texture, affected_rect, pixels_to_update);
      //     temp_arena_end(&temp);
      //   }
      // }

      /*
      // Puting the used point nodes by the last draw record back into the pool
      {
        // todo: Take a look at this again
        G->current_draw_record->last_node_with_points->next = G->first_free_node_with_points;
        Node_with_points* node_stack_head_p = G->current_draw_record->first_node_with_points;
        Node_with_points* next_p = node_stack_head_p->next;
        *node_stack_head_p = Node_with_points{};
        node_stack_head_p->next = next_p;
        // node_stack_head_p->_is_active = false;
        G->first_free_node_with_points = node_stack_head_p;
      }
      */
      
      // Puting the used image parts back into the pool
      {
        G->current_draw_record->last_image_part->next = G->first_free_image_part;
        G->first_free_image_part = G->current_draw_record->first_image_part;
      }

      Draw_record* record_to_remove = G->current_draw_record;
      G->current_draw_record = G->current_draw_record->prev;

      // Puting the used draw_record back into the pool
      {
        *record_to_remove = Draw_record{};
        // record_to_remove->_is_active = false;
        record_to_remove->_next_free = G->first_free_draw_record;
        G->first_free_draw_record = record_to_remove;
      }
    }
  }
}

void update_pencil_ui(G_state* G, RLI_Event_list* rli_events)
{
  ui_begin_build((F32)GetScreenWidth(), (F32)GetScreenHeight(), (F32)GetMouseX(), (F32)GetMouseY());

  ui_push_font(G->font_texture_for_ui);

  UI_Wrapper(Axis2__x)
  {
    ui_spacer(ui_p_of_p(1, 0));
    UI_Wrapper(Axis2__y)
    {
      ui_spacer(ui_p_of_p(1, 0));

      ui_set_next_color({ 10, 75, 107, 255 });
      UI_PaddedBox(ui_px(10), Axis2__y)
      {
        UI_Slider_style size_slider_style = {};
        size_slider_style.height         = 50;
        size_slider_style.width          = 100;
        size_slider_style.hover_color    = { 75, 75, 75, 255 };
        size_slider_style.no_hover_color = { 50, 50, 50, 255 };
        size_slider_style.text_color     = { 255, 255, 255, 255 };
        size_slider_style.slided_part_color = { 125, 125, 125, 255 };
        size_slider_style.fmt_str = "%.0f";

        U64 new_pen_size = (U64)ui_slider(Str8FromC("Size slider id"), &size_slider_style, (F32)G->pen_size, 1, 100, rli_events);
        if (new_pen_size % 2 == 0 && new_pen_size != 0) { new_pen_size -= 1; }
        clamp_u64_inplace(&new_pen_size, 1, 99);
        if (new_pen_size != G->pen_size)
        {
          G->signal_new_pen_size = true;
          G->new_pen_size = new_pen_size;
        }

        ui_spacer(ui_px(25));

        UI_Slider_style red_color_slider = {};
        red_color_slider.height         = 50;
        red_color_slider.width          = 100;
        red_color_slider.hover_color    = { 75, 75, 75, 255 };
        red_color_slider.no_hover_color = { 50, 50, 50, 255 };
        red_color_slider.text_color     = { 255, 255, 255, 255 };
        red_color_slider.slided_part_color = { 125, 125, 125, 255 };
        red_color_slider.fmt_str = "%.0f";

        F32 new_pen_red = ui_slider(Str8FromC("Red color slider"), &size_slider_style, G->pen_color.r, 0, 255, rli_events);
        G->pen_color.r = new_pen_red;

        ui_spacer(ui_px(25));

        UI_Slider_style green_color_slider = {};
        green_color_slider.height         = 50;
        green_color_slider.width          = 100;
        green_color_slider.hover_color    = { 75, 75, 75, 255 };
        green_color_slider.no_hover_color = { 50, 50, 50, 255 };
        green_color_slider.text_color     = { 255, 255, 255, 255 };
        green_color_slider.slided_part_color = { 125, 125, 125, 255 };
        green_color_slider.fmt_str = "%.0f";

        F32 new_pen_green = ui_slider(Str8FromC("Green color slider"), &size_slider_style, G->pen_color.g, 0, 255, rli_events);
        G->pen_color.g = new_pen_green;

        ui_spacer(ui_px(25));

        UI_Slider_style blue_color_slider = {};
        blue_color_slider.height         = 50;
        blue_color_slider.width          = 100;
        blue_color_slider.hover_color    = { 75, 75, 75, 255 };
        blue_color_slider.no_hover_color = { 50, 50, 50, 255 };
        blue_color_slider.text_color     = { 255, 255, 255, 255 };
        blue_color_slider.slided_part_color = { 125, 125, 125, 255 };
        blue_color_slider.fmt_str = "%.0f";

        F32 new_pen_blue = ui_slider(Str8FromC("Blue color slider"), &size_slider_style, G->pen_color.b, 0, 255, rli_events);
        G->pen_color.b = new_pen_blue;

        ui_spacer(ui_px(25));

        UI_Slider_style alpha_color_slider = {};
        alpha_color_slider.height         = 50;
        alpha_color_slider.width          = 100;
        alpha_color_slider.hover_color    = { 75, 75, 75, 255 };
        alpha_color_slider.no_hover_color = { 50, 50, 50, 255 };
        alpha_color_slider.text_color     = { 255, 255, 255, 255 };
        alpha_color_slider.slided_part_color = { 125, 125, 125, 255 };
        alpha_color_slider.fmt_str = "%.0f";

        F32 new_pen_alpha = ui_slider(Str8FromC("Aplha color slider"), &size_slider_style, G->pen_color.a, 0, 255, rli_events);
        G->pen_color.a = new_pen_alpha;
      }

      ui_spacer(ui_px(25));
      ui_spacer(ui_p_of_p(1, 0));
    }
    ui_spacer(ui_px(10));
  }

  ui_end_build();
}

int main()
{
  os_init();
  allocate_thread_context();
  os_win32_set_thread_context(get_thread_context()); // todo: This is a flaw of the os system and the fact that we link in
  RLI_init(os_get_keyboard_initial_repeat_delay(), os_get_keyboard_subsequent_repeat_delay());
  ui_init();

  ui_set_text_measuring_function(ui_text_measure_func);
  InitWindow(800, 600, "Pencil");

  G_state G = {};

  G.arena = arena_alloc(Megabytes(64));
  G.frame_arena = arena_alloc(Megabytes(64));
  G.pen_size = 11;
  G.font_texture_for_ui = LoadFont("../data/Roboto.ttf");

  G.arena_for_draw_records = arena_alloc(Megabytes(64));
  G.first_draw_record = ArenaCurrentPos(G.arena_for_draw_records, Draw_record);

  /*
  G.arena_for_nodes_with_points = arena_alloc(Megabytes(64));
  G.first_node_with_points = ArenaCurrentPos(G.arena_for_nodes_with_points, Node_with_points);
  */
  
  G.arena_for_image_parts = arena_alloc(Megabytes(64));
  G.first_image_part = ArenaCurrentPos(G.arena_for_image_parts, Image_part);

  G.draw_texture = {};
  {
    Temp_arena temp = temp_arena_begin(G.frame_arena);
    Data_buffer draw_image_buffer = data_buffer_make(temp.arena, GetScreenWidth() * GetScreenHeight() * 4);
    Image image = {};
    image.data    = draw_image_buffer.data;
    image.format  = PixelFormat_UNCOMPRESSED_R8G8B8A8;
    image.width   = GetScreenWidth();
    image.height  = GetScreenHeight();
    image.mipmaps = 1;
    bool succ = ExportImage(image, "draw_image.png");
    HandleLater(succ);
    G.draw_texture = LoadTexture("draw_image.png");
    temp_arena_end(&temp);
  }
  // note: These have to be the same, but 1 is on the gpu for quick drawing and another one is the the cpu for state managment
  G.draw_texture_after_the_last_draw = {};
  {
    G.draw_texture_after_the_last_draw.pixels = ArenaPushArr(G.arena, U32, G.draw_texture.width * G.draw_texture.height);
    G.draw_texture_after_the_last_draw.width = G.draw_texture.width;
    G.draw_texture_after_the_last_draw.height = G.draw_texture.height;
  }

  // ==============================

  for (;!WindowShouldClose();)
  {
    arena_clear(G.frame_arena);
    RLI_Event_list* rli_events = RLI_get_frame_inputs(); 
    
    // if ui has an active element which we are holding, then we dont start interacting with the app (drawing part)
    
    if (!G.is_mid_drawing)
    {
      update_pencil_ui(&G, rli_events);
    }

    update_pencil(&G, ui_has_active());

    DeferLoop(BeginDrawing(), EndDrawing())
    DeferLoop(BeginBlendMode(BLEND_ALPHA), EndBlendMode())
    {
      Texture draw_texture = G.draw_texture;
      ClearBackground(BLACK);
      DrawTexturePro(draw_texture, Rectangle{0, 0, (F32)draw_texture.width, (F32)draw_texture.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);   
      DrawFPS(0, 0);
      ui_draw(); 
      // DrawRectangle(0, 0, 500, 500, { 255, 255, 255, 255 });
    }
  }


  // note: not releasing stuff, defering it to the system
  return 0;
}