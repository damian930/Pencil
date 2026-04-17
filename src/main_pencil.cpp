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

#include "GL/gl.h"
#include "wingdi.h"
#pragma comment(lib, "opengl32.lib")
	
void _glCopyImageSubData_stub(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth)
{
  BreakPoint();
}

typedef void (*glCopyImageSubData_fp) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);  
glCopyImageSubData_fp glCopyImageSubData = _glCopyImageSubData_stub; 

V2 ui_text_measure_func(Str8 text, Font font, F32 font_size)
{ 
  Scratch scratch = get_scratch(0, 0);
  Str8 text_nt = str8_copy_alloc(scratch.arena, text);
  Vector2 rl_vec = MeasureTextEx(font, (char*)text_nt.data, font_size, 0);
  V2 result = { rl_vec.x, rl_vec.y };
  end_scratch(&scratch);
  return result;
}

///////////////////////////////////////////////////////////
// - Pencil stuff
//

#define IMAGE_PART_DATA_WIDTH  200
#define IMAGE_PART_DATA_HEIGHT 200
// struct Image_part {
//   V2U64 offset;
//   U64 width;
//   U64 height;
// };

struct Draw_record {
  U64 pen_size; // todo: I am not sure we still need this
  Draw_record* prev;

  RangeV2U64 affected_2d_range; // note: These is calculated mid drawing for quick access after we done drawing
  RenderTexture draw_texture_state_before;

  // Free list part
  Draw_record* _next_free;
};

struct G_state {
  Arena* arena;
  Arena* frame_arena;
  U64 pen_size;
  V4U8 pen_color;

  RenderTexture draw_texture_always_fresh; 
  RenderTexture draw_texture_not_that_fresh;

  // Pool of draw_records  
  Arena* arena_for_draw_records;
  Draw_record* first_draw_record;
  U64 allocated_draw_records_count;
  Draw_record* first_free_draw_record;

  // Stuff for while drawing
  B32 is_mid_drawing;
  Draw_record* current_draw_record;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;

  // Misc
  Font font_texture_for_ui;
  V2U64 last_screen_dims;
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

// Image_part* get_available_image_part_from_pool(G_state* G)
// {
//   Image_part* part = G->first_free_image_part;
//   if (part)
//   {
//     G->first_free_image_part = G->first_free_image_part->next;
//   }
//   else
//   {
//     part = ArenaPush(G->arena_for_image_parts, Image_part);
//     G->allocated_image_parts_count += 1;
//   }
//   *part = Image_part{};
//   return part;
// }

// U32* image_part_get_px(Image_part* part, U64 x, U64 y)
// {
//   Assert(x < part->width); Assert(y < part->height); 
//   U32* px = part->pixels + (y * IMAGE_PART_DATA_WIDTH + x);
//   return px;
// }

// /*
// struct Draw_result_mem {
//   RangeV2U64 update_range;
//   U32* new_pixels;
// };

// Draw_result_mem get_draw_range_memory(Arena* arena, V2U64 pos, U64 pen_size, V4U8 color)
// {
//   Assert(pen_size != 0 && pen_size % 2 == 1); // Just making sure that it is an odd value
//   Draw_result_mem result = {};
//   U32* new_pixels = ArenaPushArr(arena, U32, pen_size * pen_size);
//   for (U64 y_index = 0; y_index < pen_size; y_index += 1)
//   {
//     for (U64 x_index = 0; x_index < pen_size; x_index += 1)
//     {
//       U32* px = new_pixels + (pen_size * y_index + x_index);
//       ((U8*)(px))[0] = color.r;
//       ((U8*)(px))[1] = color.g;
//       ((U8*)(px))[2] = color.b;
//       ((U8*)(px))[3] = color.a; 
//     }
//   }
//   U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);
  
//   // todo: I just dont want to deal with this now
//   Assert(pos.x >= pixels_on_each_side_to_the_middle_pixel); // These 2 are for same minus operations down below
//   Assert(pos.y >= pixels_on_each_side_to_the_middle_pixel); // These 2 are for same minus operations down below
//   // U64 min_px_x = (pos.x >= pixels_on_each_side_to_the_middle_pixel ? pos.x - pixels_on_each_side_to_the_middle_pixel : 0);
//   // U64 min_px_y = (pos.y >= pixels_on_each_side_to_the_middle_pixel ? pos.y - pixels_on_each_side_to_the_middle_pixel : 0);

//   RangeV2U64 affected_range = {};
//   affected_range.min.x = pos.x - pixels_on_each_side_to_the_middle_pixel; 
//   affected_range.min.y = pos.y - pixels_on_each_side_to_the_middle_pixel;
//   affected_range.max.x = affected_range.min.x + pen_size;
//   affected_range.max.y = affected_range.min.y + pen_size;

//   result.new_pixels = new_pixels;
//   result.update_range = affected_range;
//   return result;
// }
// */

RangeV2U64 get_affected_points_for_draw_call(V2U64 pos, U64 pen_size, V2U64 upper_bound)
{
  Assert(pen_size % 2 == 1); // Just to catch an error 
  U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);
  
  U64 max_x = pos.x + pixels_on_each_side_to_the_middle_pixel;
  if (max_x > upper_bound.x) { max_x = upper_bound.x; }

  U64 max_y = pos.y + pixels_on_each_side_to_the_middle_pixel;
  if (max_y > upper_bound.y) { max_y = upper_bound.y; }

  U64 min_x = {};
  if (pos.x < pixels_on_each_side_to_the_middle_pixel) {
    min_x = 0;
  } else {
    min_x = pos.x - pixels_on_each_side_to_the_middle_pixel;
  }

  U64 min_y = {};
  if (pos.y < pixels_on_each_side_to_the_middle_pixel) {
    min_y = 0;
  } else {
    min_y = pos.y - pixels_on_each_side_to_the_middle_pixel;
  }

  RangeV2U64 affected_range = {};
  affected_range.min.x = min_x;
  affected_range.min.y = min_y;
  affected_range.max.x = max_x;
  affected_range.max.y = max_y;
  return affected_range;
}

void draw_onto_texture(RenderTexture texture, V2U64 pos, U64 pen_size, V4U8 color)
{
  Assert(pen_size % 2 == 1); // Just to catch an error 
  U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);

  BeginTextureMode(texture);
  DrawCircle((int)pos.x, texture.texture.height - (int)pos.y, (F32)pixels_on_each_side_to_the_middle_pixel, { color.r, color.g, color.b, color.a });
  EndTextureMode();
}

void copy_from_texture_to_texture(
  Texture dest, V2U64 dest_offset, 
  Texture src, V2U64 src_offset, 
  U64 width_to_copy,
  U64 height_to_copy
) {
  glCopyImageSubData(
    src.id, GL_TEXTURE_2D, 0, (GLint)src_offset.x, (GLint)src_offset.y, 0,
    dest.id, GL_TEXTURE_2D, 0, (GLint)dest_offset.x, (GLint)dest_offset.y, 0,
    (GLsizei)width_to_copy, (GLsizei)height_to_copy, 1
  );
}

V2U64 get_draw_texture_dims(const G_state* G)
{
  Assert(G->draw_texture_always_fresh.texture.width == G->draw_texture_not_that_fresh.texture.width);
  Assert(G->draw_texture_always_fresh.texture.height == G->draw_texture_not_that_fresh.texture.height);
  int w = G->draw_texture_always_fresh.texture.width;  Assert(w >= 0);
  int h = G->draw_texture_always_fresh.texture.height; Assert(h >= 0);
  V2U64 dims = v2u64((U64)w, (U64)h);
  return dims;
}

U64 get_draw_texture_width(const G_state* G)
{
  return get_draw_texture_dims(G).x;
}

U64 get_draw_texture_height(const G_state* G)
{
  return get_draw_texture_dims(G).y;
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
    G->is_mid_drawing = false;

    U64 width = G->draw_texture_always_fresh.texture.width;
    U64 height = G->draw_texture_always_fresh.texture.height;

    RenderTexture2D old_copy_texture = LoadRenderTexture((int)width, (int)height);
    HandleLater(old_copy_texture.texture.id != 0);

    copy_from_texture_to_texture(
      old_copy_texture.texture, v2u64(0, 0), 
      G->draw_texture_not_that_fresh.texture, v2u64(0, 0),
      width, height
    );
    G->current_draw_record->draw_texture_state_before = old_copy_texture;

    // Updating the old texture to be the same as the fresh texture
    copy_from_texture_to_texture(
      G->draw_texture_not_that_fresh.texture, v2u64(0, 0), 
      G->draw_texture_always_fresh.texture, v2u64(0, 0),
      width, height
    );

    // Debug
    {
      Image gpu_image_fresh = LoadImageFromTexture(G->draw_texture_always_fresh.texture);
      Assert(gpu_image_fresh.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
      
      Image gpu_image_other = LoadImageFromTexture(G->draw_texture_not_that_fresh.texture);
      Assert(gpu_image_other.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);

      Image copy = LoadImageFromTexture(old_copy_texture.texture);
      Assert(copy.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);

      ExportImage(gpu_image_fresh, "fresh.png");
      ExportImage(gpu_image_other, "other.png");
      ExportImage(copy, "copy.png");
    }
  }
  else 
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) // User is about to start drawing or already drawing
  {
    // Starting to draw a new record
    if (!G->is_mid_drawing && !is_ui_capturing_mouse) 
    { 
      G->is_mid_drawing = true;
    
      Draw_record* new_draw_record = get_available_draw_record_from_pool(G);
      new_draw_record->prev = G->current_draw_record; 

      G->current_draw_record = new_draw_record;
      G->current_draw_record->pen_size = G->pen_size; // todo: this might not be needed now

      G->current_draw_record->affected_2d_range.min = v2u64(get_draw_texture_width(G), get_draw_texture_height(G));
      G->current_draw_record->affected_2d_range.max = v2u64(0, 0);
    }

    // Updating the draw state
    if (G->is_mid_drawing) 
    {
      Assert(G->pen_size != 0 && G->pen_size % 2 == 1); // Just making sure that it is an odd value

      V2U64 mouse_pos = v2u64((U64)GetMouseX(), (U64)GetMouseY());
      
      // Updating the texture
      RangeV2U64 affected_range = get_affected_points_for_draw_call(mouse_pos, G->pen_size, G->last_screen_dims);
      draw_onto_texture(G->draw_texture_always_fresh, mouse_pos, G->pen_size, G->pen_color);

      // Updating affected range min value
      V2U64 af_min = affected_range.min;
      V2U64* state_min_point = &G->current_draw_record->affected_2d_range.min;
      state_min_point->x = Min(state_min_point->x, affected_range.min.x);
      state_min_point->y = Min(state_min_point->y, affected_range.min.y);

      // Updating affected range max value
      V2U64 af_max = affected_range.max;
      V2U64* state_max_point = &G->current_draw_record->affected_2d_range.max;
      state_max_point->x = Max(state_max_point->x, affected_range.max.x);
      state_max_point->y = Max(state_max_point->y, affected_range.max.y);
    }
    // todo: Update this comment here (We no longer use cpu side image)
    // note: Here is the only time when the cpu side image is not synced to the gpu one. 
    //       But we do later sync it when we stop drawing.
  }
  else 
  if (IsKeyPressed(KEY_BACKSPACE)) // User want to remove the last line they drew
  {
    if (G->current_draw_record)
    {
      Assert(G->current_draw_record->pen_size % 2 == 1); // Hard to draw pen_size, which is the diameter when we press on the middle px, but there is not middle pixel, since the diameter is an even value
    
      U64 w = G->draw_texture_always_fresh.texture.width;
      U64 h = G->draw_texture_always_fresh.texture.height;
      // Removing the record from draw_texure
      copy_from_texture_to_texture(G->draw_texture_always_fresh.texture, v2u64(0, 0), G->current_draw_record->draw_texture_state_before.texture, v2u64(0, 0), w, h);
      // Drawing back the stuff that was on the removed image parts before the last draw record
      copy_from_texture_to_texture(G->draw_texture_not_that_fresh.texture, v2u64(0, 0), G->current_draw_record->draw_texture_state_before.texture, v2u64(0, 0), w, h);

      Draw_record* record_to_remove = G->current_draw_record;
      G->current_draw_record = G->current_draw_record->prev;
      UnloadRenderTexture(record_to_remove->draw_texture_state_before); 

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
  
  // todo: This is not great
  // SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_BORDERLESS_WINDOWED_MODE);
  // InitWindow(1920, 1080, "Pencil");
  
  InitWindow(800, 600, "Pencil");

  glCopyImageSubData = (glCopyImageSubData_fp)wglGetProcAddress("glCopyImageSubData");
  HandleLater(glCopyImageSubData != (void*)0 && glCopyImageSubData != (void*)1 && glCopyImageSubData != (void*)2 && glCopyImageSubData != (void*)3 && glCopyImageSubData != (void*)-1);

  // Gpu test
  /*
  InitWindow(800, 600, "Pencil");

  glCopyImageSubData = (glCopyImageSubData_fp)wglGetProcAddress("glCopyImageSubData");
  HandleLater(glCopyImageSubData != (void*)1 && glCopyImageSubData != (void*)2 && glCopyImageSubData != (void*)3 && glCopyImageSubData != (void*)-1);

  Texture t1 = LoadTexture("F:/red.png");
  Texture t2 = LoadTexture("F:/blue.png");
  for (;!WindowShouldClose();)
  {
    static Image_part part = {};
    part.height = 100;
    part.width = 100;
    part.offset = v2u64(50, 50);

    static B32 done = false;
    if (!done)
    {
      done = true;
      copy_from_texture_to_texture(t1, part.offset, t2, part.offset, part.width, part.height);
    }

    BeginDrawing();
    {
      DrawTexturePro(t1, Rectangle{0, 0, (F32)t1.width, (F32)t1.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);
      DrawTexturePro(t2, Rectangle{0, 0, (F32)t2.width, (F32)t2.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);
    }
    EndDrawing();
  }
  */

  G_state G = {};
  
  G.arena = arena_alloc(Megabytes(64));
  G.frame_arena = arena_alloc(Megabytes(64));
  G.pen_size = 11;
  G.pen_color = v4u8(255, 255, 255, 255);

  G.draw_texture_always_fresh = LoadRenderTexture(GetScreenWidth(), GetScreenHeight()); 
  Assert(G.draw_texture_always_fresh.id != 0);
  BeginTextureMode(G.draw_texture_always_fresh); { ClearBackground(BLANK); } EndTextureMode(); 

  G.draw_texture_not_that_fresh = LoadRenderTexture(GetScreenWidth(), GetScreenHeight()); 
  Assert(G.draw_texture_not_that_fresh.id != 0);
  BeginTextureMode(G.draw_texture_not_that_fresh); { ClearBackground(BLANK); } EndTextureMode(); 

  G.arena_for_draw_records = arena_alloc(Megabytes(64));
  G.first_draw_record = ArenaCurrentPos(G.arena_for_draw_records, Draw_record);

  G.font_texture_for_ui = LoadFont("../data/Roboto.ttf");
  G.last_screen_dims = {};
  {
    int w = GetScreenWidth();  Assert(w >= 0);
    int h = GetScreenHeight(); Assert(h >= 0);
    G.last_screen_dims = v2u64((U64)w, (U64)h);
  }

  // ==============================

  for (;!WindowShouldClose();)
  {
    // Checking if the app invariant is still valid
    {
      V2U64 test_dims = v2u64((U64)GetScreenWidth(), (U64)GetScreenHeight());
      if (!v2u64_match(G.last_screen_dims, test_dims))
      {
        InvalidCodePath();
        // note: This logic is not implemented yet
      }
    }

    arena_clear(G.frame_arena);
    RLI_Event_list* rli_events = RLI_get_frame_inputs(); 
    
    if (!G.is_mid_drawing) {
      update_pencil_ui(&G, rli_events);
    }

    update_pencil(&G, ui_has_active());

    #define DEBUG_CHECK_IF_TEXTURES_ARE_VALID 0
    #if DEBUG_CHECK_IF_TEXTURES_ARE_VALID
    {
      if (!G.is_mid_drawing) // The textures are legaly un synched when drawing
      {
        Image gpu_image_fresh = LoadImageFromTexture(G.draw_texture_always_fresh.texture);
        Assert(gpu_image_fresh.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
        
        Image gpu_image_other = LoadImageFromTexture(G.draw_texture_not_that_fresh.texture);
        Assert(gpu_image_other.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
        
        Assert(gpu_image_other.width == gpu_image_fresh.width);
        Assert(gpu_image_other.height == gpu_image_fresh.height);
        
        B32 is_the_same = true;
        for (U64 y_index = 0; y_index < gpu_image_fresh.height; y_index += 1)
        {
          for (U64 x_index = 0; x_index < gpu_image_fresh.width; x_index += 1)
          {
            U32 px1 = ((U32*)(gpu_image_fresh.data))[y_index * gpu_image_fresh.width + x_index];
            U32 px2 = ((U32*)(gpu_image_other.data))[y_index * gpu_image_fresh.width + x_index];
            if (px1 != px2)
            {
              is_the_same = false;
              break;
            }
          }
        }

        if (!is_the_same)
        {
          B32 succ = true;
          succ &= (B32)ExportImage(gpu_image_fresh, "fresh.png");
          succ &= (B32)ExportImage(gpu_image_other, "other.png");
          BreakPoint();
        }
      }
    }
    #endif
    #undef DEBUG_CHECK_IF_TEXTURES_ARE_VALID

    DeferLoop(BeginDrawing(), EndDrawing())
    {
      ClearBackground(BLANK);
      
      DrawTexture(G.draw_texture_always_fresh.texture, 0, 0, WHITE);
      
      ui_draw(); 
      
      DrawFPS(0, 0);
    }
  }
  
  // note: not releasing stuff, defering it to the system
  return 0;
}

