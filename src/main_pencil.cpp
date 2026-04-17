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
  U64 pen_size; // todo: I am not sure we still need this

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
  V4U8 pen_color;

  Texture draw_texture;
  Image_data draw_texture_after_the_last_draw; // Cpu side draw_texture  

  // Pool of draw_records  
  Arena* arena_for_draw_records;
  Draw_record* first_draw_record;
  U64 allocated_draw_records_count;
  Draw_record* first_free_draw_record;

  // Pool of Image parts
  Arena* arena_for_image_parts;
  Image_part* first_image_part;
  U64 allocated_image_parts_count;
  Image_part* first_free_image_part;

  // Stuff for while drawing
  B32 is_mid_drawing;
  Draw_record* current_draw_record;
  //
  Arena* arena_for_draw_poitns;
  V2U64* first_draw_point;
  U64 draw_points_count;
  //
  V2U64 last_point_where_drew;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;

  // Misc
  Font font_texture_for_ui;
  // V2U64 last_screen_dims;
};

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
  Assert(x < part->width); Assert(y < part->height); 
  U32* px = part->pixels + (y * IMAGE_PART_DATA_WIDTH + x);
  return px;
}

struct Draw_result_mem {
  RangeV2U64 update_range;
  U32* new_pixels;
};

Draw_result_mem get_draw_range_memory(Arena* arena, V2U64 pos, U64 pen_size, V4U8 color)
{
  Assert(pen_size != 0 && pen_size % 2 == 1); // Just making sure that it is an odd value
  Draw_result_mem result = {};
  U32* new_pixels = ArenaPushArr(arena, U32, pen_size * pen_size);
  for (U64 y_index = 0; y_index < pen_size; y_index += 1)
  {
    for (U64 x_index = 0; x_index < pen_size; x_index += 1)
    {
      U32* px = new_pixels + (pen_size * y_index + x_index);
      ((U8*)(px))[0] = color.r;
      ((U8*)(px))[1] = color.g;
      ((U8*)(px))[2] = color.b;
      ((U8*)(px))[3] = color.a; 
    }
  }
  U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);
  
  // todo: I just dont want to deal with this now
  Assert(pos.x >= pixels_on_each_side_to_the_middle_pixel); // These 2 are for same minus operations down below
  Assert(pos.y >= pixels_on_each_side_to_the_middle_pixel); // These 2 are for same minus operations down below
  // U64 min_px_x = (pos.x >= pixels_on_each_side_to_the_middle_pixel ? pos.x - pixels_on_each_side_to_the_middle_pixel : 0);
  // U64 min_px_y = (pos.y >= pixels_on_each_side_to_the_middle_pixel ? pos.y - pixels_on_each_side_to_the_middle_pixel : 0);

  RangeV2U64 affected_range = {};
  affected_range.min.x = pos.x - pixels_on_each_side_to_the_middle_pixel; 
  affected_range.min.y = pos.y - pixels_on_each_side_to_the_middle_pixel;
  affected_range.max.x = affected_range.min.x + pen_size;
  affected_range.max.y = affected_range.min.y + pen_size;

  result.new_pixels = new_pixels;
  result.update_range = affected_range;
  return result;
}

void update_pencil(G_state* G, B32 is_ui_capturing_mouse)
{
  Assert(G->first_draw_record != 0);
  
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
    
    U64 n_parts_we_need_on_x = u64_divide_and_ceil(count_affectd_px_on_x, IMAGE_PART_DATA_WIDTH);
    U64 n_parts_we_need_on_y = u64_divide_and_ceil(count_affectd_px_on_y, IMAGE_PART_DATA_HEIGHT);

    Image_part** image_parts_arr = ArenaPushArr(G->frame_arena, Image_part*, n_parts_we_need_on_x * n_parts_we_need_on_y);

    // Allocating parts to cover the affected area
    U64 count_affectd_px_on_y_mutable = count_affectd_px_on_y;
    for (U64 part_index_y = 0; part_index_y < n_parts_we_need_on_y; part_index_y += 1)
    {
      U64 count_affectd_px_on_x_mutable = count_affectd_px_on_x;
      
      U64 this_x_loop_y_px_left = 0;
      if (count_affectd_px_on_y_mutable > IMAGE_PART_DATA_HEIGHT) {
        this_x_loop_y_px_left = IMAGE_PART_DATA_HEIGHT;
        count_affectd_px_on_y_mutable -= IMAGE_PART_DATA_HEIGHT;
      } else {
        this_x_loop_y_px_left = count_affectd_px_on_y_mutable;
        count_affectd_px_on_y_mutable -= count_affectd_px_on_y_mutable; 
      }

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
        new_part->height = this_x_loop_y_px_left;

        // note: Data will be set in the next routine
      }
    }
    Assert(count_affectd_px_on_y_mutable == 0);

    // Iterating over draw image pixels and storing them into image parts 
    for (
      U64 draw_texture_affected_px_index_y = affected_range.min.y; 
      draw_texture_affected_px_index_y < affected_range.max.y; 
      draw_texture_affected_px_index_y += 1
    ) {
      for (
        U64 draw_texture_affected_px_index_x = affected_range.min.x; 
        draw_texture_affected_px_index_x < affected_range.max.x; 
        draw_texture_affected_px_index_x += 1
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

    // Iterating over stored draw points and updating the cpu draw image based on them
    for (U64 point_index = 0; point_index < G->draw_points_count; point_index += 1)
    {
      Temp_arena temp = temp_arena_begin(G->frame_arena);
      {
        V2U64 point = G->first_draw_point[point_index];
        Draw_result_mem draw_mem = get_draw_range_memory(temp.arena, point, G->pen_size, G->pen_color);
        Image_data* image = &G->draw_texture_after_the_last_draw;
        RangeV2U64 updated_range = draw_mem.update_range;
        for (U64 y_index = updated_range.min.y; y_index < updated_range.max.y; y_index += 1)
        {
          for (U64 x_index = updated_range.min.x; x_index < updated_range.max.x; x_index += 1)
          {
            U32* px_value = image->pixels + (y_index * image->width + x_index);
            ((U8*)(px_value))[0] = G->pen_color.r;
            ((U8*)(px_value))[1] = G->pen_color.g;
            ((U8*)(px_value))[2] = G->pen_color.b;
            ((U8*)(px_value))[3] = G->pen_color.a;
          }
        }
      }
      temp_arena_end(&temp);
    }
  }
  else 
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) // User is about to start drawing or already drawing
  {
    // Starting to draw a new record
    if (!G->is_mid_drawing && !is_ui_capturing_mouse) 
    { 
      G->is_mid_drawing = true;
    
      // todo: This shoud be reset after we dont updating the cpu side image and not here
      arena_clear(G->arena_for_draw_poitns);
      G->draw_points_count = 0;
      G->last_point_where_drew = v2u64(u64_max, u64_max); // Just using an unreachable value

      Draw_record* new_draw_record = get_available_draw_record_from_pool(G);
      new_draw_record->prev = G->current_draw_record; 

      G->current_draw_record = new_draw_record;
      G->current_draw_record->pen_size = G->pen_size; // todo: this might not be needed now

      G->current_draw_record->affected_2d_range.min = v2u64(G->draw_texture.width, G->draw_texture.height);
      G->current_draw_record->affected_2d_range.max = v2u64(0, 0);
    }

    // Updating the draw state
    if (G->is_mid_drawing) 
    {
      // todo: Use a u64 vector to not have to cast each time you use this 
      V2U64 mouse_pos = v2u64((U64)GetMouseX(), (U64)GetMouseY());

      Assert(G->pen_size != 0 && G->pen_size % 2 == 1); // Just making sure that it is an odd value
      Assert(G->first_draw_point); // This shoud always be a valid pointer by the way, so just checking
      
      // Drawing
      if (!v2u64_match(G->last_point_where_drew, mouse_pos))
      {
        G->last_point_where_drew = mouse_pos;

        // todo: Stop manually upadting the min max, derive it from the single source of truth: from get_draw_range_memory down below
        
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
        {
          Temp_arena temp = temp_arena_begin(G->frame_arena);
          Draw_result_mem new_draw_mem = get_draw_range_memory(temp.arena, mouse_pos, G->pen_size, G->pen_color);
          Rectangle raylib_rect = {};
          raylib_rect.x      = (F32)(new_draw_mem.update_range.min.x);
          raylib_rect.y      = (F32)(new_draw_mem.update_range.min.y);
          raylib_rect.width  = (F32)(new_draw_mem.update_range.max.x - new_draw_mem.update_range.min.x);
          raylib_rect.height = (F32)(new_draw_mem.update_range.max.y - new_draw_mem.update_range.min.y);
          UpdateTextureRec(G->draw_texture, raylib_rect, new_draw_mem.new_pixels);
          temp_arena_end(&temp);
        }

        // note: Here is the only time when the cpu side image is not synced to the gpu one. 
        //       But we do later sync it when we stop drawing.

        // Storing the new point for later update of the cpu side draw image
        V2U64* new_draw_p = ArenaPush(G->arena_for_draw_poitns, V2U64);
        G->draw_points_count += 1;
        *new_draw_p = mouse_pos;
      }
    }
  } 
  else 
  if (IsKeyPressed(KEY_BACKSPACE)) // User want to remove the last line they drew
  {
    if (G->current_draw_record)
    {
      Assert(G->current_draw_record->pen_size % 2 == 1); // Hard to draw pen_size, which is the diameter when we press on the middle px, but there is not middle pixel, since the diameter is an even value
    
      // Removing the record from draw_texure
      for (Image_part* part = G->current_draw_record->first_image_part; part; part = part->next)
      {
        if (part->width == IMAGE_PART_DATA_WIDTH && part->height == IMAGE_PART_DATA_HEIGHT)
        {
          Rectangle rect = { (F32)part->offset.x, (F32)part->offset.y, (F32)part->width, (F32)part->height };
          UpdateTextureRec(G->draw_texture, rect, part->pixels);
        }
        else // Have to redo the part pixels to have the stride be valid 
        {
          Temp_arena temp = temp_arena_begin(G->frame_arena);
          U32* fixed_stride_pixel_buffer = ArenaPushArr(temp.arena, U32, part->width * part->height);
          for (U64 y_index = 0; y_index < part->height; y_index += 1)
          {
            for (U64 x_index = 0; x_index < part->width; x_index += 1)
            {
              U32* valid_stride_px = fixed_stride_pixel_buffer + (y_index * part->width + x_index);
              U32* part_px = image_part_get_px(part, x_index, y_index); 
              *valid_stride_px = *part_px;
            }
          }
          Rectangle rect = { (F32)part->offset.x, (F32)part->offset.y, (F32)part->width, (F32)part->height };
          UpdateTextureRec(G->draw_texture, rect, fixed_stride_pixel_buffer);
          temp_arena_end(&temp);
        }
      }

      // Drawing back the stuff that was on the removed image parts before the last draw record
      for (Image_part* part = G->current_draw_record->first_image_part; part; part = part->next)
      {
        U64 cpu_side_image_w = G->draw_texture_after_the_last_draw.width;
        U32* cpu_side_image = G->draw_texture_after_the_last_draw.pixels;
        U32* cpu_side_first_px = cpu_side_image + (part->offset.y * cpu_side_image_w + part->offset.x);
        for (U64 y_index = 0; y_index < part->height; y_index += 1)
        {
          for (U64 x_index = 0; x_index < part->width; x_index += 1)
          {
            cpu_side_first_px[x_index] = *image_part_get_px(part, x_index, y_index);
          }
          cpu_side_first_px += cpu_side_image_w;
        }
      }

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

        U8 new_pen_red = (U8)ui_slider(Str8FromC("Red color slider"), &size_slider_style, G->pen_color.r, 0, 255, rli_events);
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

        U8 new_pen_green = (U8)ui_slider(Str8FromC("Green color slider"), &size_slider_style, G->pen_color.g, 0, 255, rli_events);
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

        U8 new_pen_blue = (U8)ui_slider(Str8FromC("Blue color slider"), &size_slider_style, G->pen_color.b, 0, 255, rli_events);
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

        U8 new_pen_alpha = (U8)ui_slider(Str8FromC("Aplha color slider"), &size_slider_style, G->pen_color.a, 0, 255, rli_events);
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
  // SetWindowState(FLAG_WINDOW_TOPMOST);
  // SetWindowState(FLAG_WINDOW_UNDECORATED);
  // SetWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
  // SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);

  G_state G = {};

  G.arena = arena_alloc(Megabytes(64));
  G.frame_arena = arena_alloc(Megabytes(64));
  G.pen_size = 11;
  G.font_texture_for_ui = LoadFont("../data/Roboto.ttf");

  G.arena_for_draw_records = arena_alloc(Megabytes(64));
  G.first_draw_record = ArenaCurrentPos(G.arena_for_draw_records, Draw_record);

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

  G.arena_for_draw_poitns = arena_alloc(Megabytes(64));
  G.first_draw_point = ArenaCurrentPos(G.arena_for_draw_poitns, V2U64);

  // ==============================

  for (;!WindowShouldClose();)
  {
    arena_clear(G.frame_arena);
    RLI_Event_list* rli_events = RLI_get_frame_inputs(); 
    
    if (!G.is_mid_drawing)
    {
      update_pencil_ui(&G, rli_events);
    }

    update_pencil(&G, ui_has_active());
   
    #define DEBUG_CHECK_IF_TEXTURES_ARE_VALID 0
    #if DEBUG_CHECK_IF_TEXTURES_ARE_VALID
    {
      if (!G.is_mid_drawing) // The textures are legaly un synched when drawing
      {
        Image gpu_image = LoadImageFromTexture(G.draw_texture);
        Assert(gpu_image.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
        Assert(gpu_image.width == G.draw_texture_after_the_last_draw.width);
        Assert(gpu_image.height == G.draw_texture_after_the_last_draw.height);
        for (U64 i = 0; i < gpu_image.height; i += 1)
        {
          for (U64 j = 0; j < gpu_image.width; j += 1)
          {
            U32 gpu_px = ((U32*)gpu_image.data)[i * gpu_image.width + j];
            U32 cpu_px = G.draw_texture_after_the_last_draw.pixels[i * gpu_image.width + j];

            if (gpu_px != cpu_px)
            {
              Image cpu_image = {};
              cpu_image.data    = G.draw_texture_after_the_last_draw.pixels;             
              cpu_image.width   = (int)G.draw_texture_after_the_last_draw.width;            
              cpu_image.height  = (int)G.draw_texture_after_the_last_draw.height;           
              cpu_image.mipmaps = 1;          
              cpu_image.format  = PixelFormat_UNCOMPRESSED_R8G8B8A8;           
  
              B32 succ = 1;
              succ &= (B32)ExportImage(gpu_image, "gpu_image.png");
              succ &= (B32)ExportImage(cpu_image, "cpu_image.png");
              Assert(succ);
              BreakPoint();
            }
          }
        }
        UnloadImage(gpu_image);
      }
    }
    #endif
    #undef DEBUG_CHECK_IF_TEXTURES_ARE_VALID

    Texture screen_texture = {};
    {
      Image image = {};
      OS_Image os_image = os_take_screenshot(G.frame_arena);
      image.data    = os_image.data;
      image.width   = (int)os_image.width;
      image.height  = (int)os_image.height;
      image.mipmaps = 1;
      image.format  = PixelFormat_UNCOMPRESSED_R8G8B8A8;
      screen_texture = LoadTextureFromImage(image);
    }
    
    DeferLoop(BeginDrawing(), EndDrawing())
    DeferLoop(BeginBlendMode(BLEND_ALPHA), EndBlendMode())
    {
      Texture draw_texture = G.draw_texture;
      ClearBackground(BLACK);
      DrawTexturePro(screen_texture, Rectangle{0, 0, (F32)screen_texture.width, (F32)screen_texture.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);
      DrawTexturePro(draw_texture, Rectangle{0, 0, (F32)draw_texture.width, (F32)draw_texture.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);   
      DrawFPS(0, 0);
      ui_draw(); 
    }

    UnloadTexture(screen_texture);
  }


  // note: not releasing stuff, defering it to the system
  return 0;
}