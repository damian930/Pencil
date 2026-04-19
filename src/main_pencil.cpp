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
#define LoadImage Win32LoadImage
#include "windows.h"
#undef LoadImage
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

enum Operation_kind {
  Operation_kind__Line,   
  Operation_kind__Eraser,
  Operation_kind__Clear_the_screen,
};

struct Draw_record {
  RenderTexture texture_before_we_affected; // This is allocated when done drawing
  RenderTexture texture_after_we_affected;  // This is allocated when done drawing

  // These are also used for draw record free list
  Draw_record* next;
  Draw_record* prev;
};

struct G_state {
  Arena* arena;
  Arena* frame_arena;
  
  U64 pen_size;
  V4U8 pen_color;

  U64 eraser_size;

  U64 draw_texures_width;
  U64 draw_texures_height;
  RenderTexture draw_texture_always_fresh; 
  RenderTexture draw_texture_not_that_fresh;

  // Pool of draw records
  #define DRAW_RECORDS_MAX_COUNT 50
  Draw_record pool_of_draw_records[DRAW_RECORDS_MAX_COUNT];
  U64 count_of_pool_draw_records_in_use; // This inludes if they are in the free list
  Draw_record* first_free_draw_record;
  Draw_record* last_free_draw_record;
  
  Draw_record* first_record;
  Draw_record* last_record;
  Draw_record* current_record;
  // Operation_kind

  // Stuff for while drawing
  B32 is_mid_drawing;
  B32 is_erasing_mode;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;
  //
  B32 signal_new_eraser_size;
  U64 new_eraser_size;
  //
  B32 signal_swap_to_eraser;
  B32 signal_swap_to_pen;

  // Misc
  Font font_texture_for_ui;
  V2U64 last_screen_dims;
  B32 show_brush_ui_menu;
  Str8 brush_menu_ui_id;
};

Draw_record* get_new_draw_record_from_pool__nullable(G_state* G)
{
  Draw_record* result = 0;
  if (G->first_free_draw_record)
  {
    result = G->first_free_draw_record;
    DllPopFront_Name(G, first_free_draw_record, last_free_draw_record, next, prev);
    *result = Draw_record{};
  }
  else if (G->count_of_pool_draw_records_in_use < DRAW_RECORDS_MAX_COUNT)
  {
    result = G->pool_of_draw_records + G->count_of_pool_draw_records_in_use;
    G->count_of_pool_draw_records_in_use += 1;
    *result = Draw_record{};
  }
  return result;
}

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

void fill_texture_with_color(RenderTexture texture, V4U8 color, B32 is_erasing)
{
  if (is_erasing) { glDisable(GL_BLEND); }
  BeginTextureMode(texture);

  ClearBackground({ color.r, color.g, color.b, color.a });

  EndTextureMode();
  if (is_erasing) { glEnable(GL_BLEND); }
}

void draw_onto_texture(RenderTexture texture, Vector2 new_pos, Vector2 prev_pos, U64 pen_size, V4U8 color, B32 is_erasing)
{
  if (is_erasing) { glDisable(GL_BLEND); }
  BeginTextureMode(texture);
  
  F32 dx = new_pos.x - prev_pos.x;
  F32 dy = new_pos.y - prev_pos.y;
  F32 length = sqrtf(dx * dx + dy * dy);
  U64 steps = (U64)length;
  for (U64 i = 0; i <= steps; i++) 
  {
    F32 t = (steps == 0) ? 0.0f : (F32)i / steps;
    F32 x = prev_pos.x + dx * t;
    F32 y = prev_pos.y + dy * t;
    DrawCircleV({ x, texture.texture.height - y }, (F32)pen_size, { color.r, color.g, color.b, color.a });
  }

  EndTextureMode();
  if (is_erasing) { glEnable(GL_BLEND); }
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

void _debug_load_gpu_textures_onto_cpu(const G_state* G)
{
  Image gpu_image_fresh = LoadImageFromTexture(G->draw_texture_always_fresh.texture);
  Assert(gpu_image_fresh.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
  
  Image gpu_image_other = LoadImageFromTexture(G->draw_texture_not_that_fresh.texture);
  Assert(gpu_image_other.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);

  Assert(gpu_image_other.width == gpu_image_fresh.width);
  Assert(gpu_image_other.height == gpu_image_fresh.height);
  
  Texture texture_before_we_affected = {};
  Texture texture_after_we_affected = {};

  if (G->current_record != 0)
  {
    texture_before_we_affected = G->current_record->texture_before_we_affected.texture;
    texture_after_we_affected = G->current_record->texture_after_we_affected.texture;
  }

  Image image_before_we_affected = LoadImageFromTexture(texture_before_we_affected);
  Assert(gpu_image_other.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
  
  Image image_after_we_affected = LoadImageFromTexture(texture_after_we_affected);
  Assert(gpu_image_other.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);

  ExportImage(gpu_image_fresh, "fresh.png");
  ExportImage(gpu_image_other, "other.png");
  ExportImage(image_before_we_affected, "before_we_affected.png");
  ExportImage(image_after_we_affected, "after_we_affected.png");
}

struct Draw_record_registration_result {
  B32 succ;
  Draw_record* record;  
};

// todo: Does this really need to care about is_ui_capturing_mouse, or shoud this
//       be handled by the caller ????
Draw_record_registration_result register_new_draw_record(G_state* G, B32 is_ui_capturing_mouse)
{
  if (G->is_mid_drawing || is_ui_capturing_mouse) { return Draw_record_registration_result{}; }

  // Freeing all the records that are in front of the current one
  if (G->current_record != 0)
  {
    for (Draw_record* record = G->last_record; record != 0;) 
    {
      if (record == G->current_record) { break; }
      UnloadRenderTexture(record->texture_before_we_affected);
      UnloadRenderTexture(record->texture_after_we_affected);
      
      DllPopBack_Name(G, first_record, last_record, next, prev);
      Draw_record* prev_record = record->prev;
      
      *record = Draw_record{};
      DllPushBack_Name(G, record, first_free_draw_record, last_free_draw_record, next, prev);
      
      record = prev_record;
    }
  }

  Draw_record* new_draw_record = get_new_draw_record_from_pool__nullable(G);
  if (new_draw_record == 0)
  {
    Draw_record* oldest_record =  G->first_record;
    DllPopFront_Name(G, first_record, last_record, next, prev);
    
    *oldest_record = Draw_record{};
    DllPushBack_Name(G, oldest_record, first_free_draw_record, last_free_draw_record, next, prev);

    new_draw_record = get_new_draw_record_from_pool__nullable(G);
  }
  Assert(new_draw_record != 0); // This has to be true, its an ivariant

  // Adding the new draw record to the draw record queue
  DllPushBack_Name(G, new_draw_record, first_record, last_record, next, prev);  Assert(G->last_record == new_draw_record);

  new_draw_record->texture_before_we_affected = LoadRenderTexture((int)G->draw_texures_width, (int)G->draw_texures_height);
  HandleLater(new_draw_record->texture_before_we_affected.texture.id != 0);

  new_draw_record->texture_after_we_affected = LoadRenderTexture((int)G->draw_texures_width, (int)G->draw_texures_height);
  HandleLater(new_draw_record->texture_after_we_affected.texture.id != 0);

  G->current_record = new_draw_record;

  Draw_record_registration_result result = {};
  result.succ = true;
  result.record = new_draw_record;

  return result;
}

void update_pencil(G_state* G, B32 is_ui_capturing_mouse)
{
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
    else
    if (G->signal_new_eraser_size)
    {
      Assert(G->is_mid_drawing == false); // Just making sure
      if (!G->is_mid_drawing)
      {
        G->eraser_size = G->new_eraser_size;
        G->signal_new_eraser_size = false;
        G->new_eraser_size = 0;
      }
    }
    else 
    if (G->signal_swap_to_eraser)
    {
      Assert(G->is_mid_drawing == false); // Just making sure
      if (!G->is_mid_drawing)
      {
        G->signal_swap_to_eraser = false;
        G->is_erasing_mode = true;
      }
    }
    else 
    if (G->signal_swap_to_pen)
    {
      Assert(G->is_mid_drawing == false); // Just making sure
      if (!G->is_mid_drawing)
      {
        G->signal_swap_to_pen = false;
        G->is_erasing_mode = false;
      }
    }
  }
  
  // idea: Based on some shortcutes, figure out the next op kind

  // Some single frame draw ops
  // 
  // User want to go back to the drawing they have last removed
  
  B32 dont_start_drawing_this_frame = false;
  // if (IsKeyPressed(KEY_S))
  // {
  //  To have a screen shot we would have to have access to the screen texture and then mix the 
  //  fresh texture with the screen texture, for that we would need to use d3d and not rayli.
  //  So for now there is not really a great way to do it. 
  // }
  if (!G->is_mid_drawing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)) 
  {
    dont_start_drawing_this_frame = true; 

    // This is here to be able to deal with current_record beeing 0
    Draw_record* next_record = 0;
    if (G->current_record == 0) {
      next_record = G->first_record;
    } 
    else if (G->current_record->next != 0) {
      next_record = G->current_record->next;
    }

    if (next_record)
    {
      RenderTexture future_texture = next_record->texture_after_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(
        G->draw_texture_always_fresh.texture, v2u64(0, 0), 
        future_texture.texture, v2u64(0, 0), 
        (U64)future_texture.texture.width, (U64)future_texture.texture.height
      );

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(
        G->draw_texture_not_that_fresh.texture, v2u64(0, 0), 
        future_texture.texture, v2u64(0, 0), 
        (U64)future_texture.texture.width, (U64)future_texture.texture.height
      );

      G->current_record = next_record;
    }
  }
  else // User wants to remove the last line they drew
  if (!G->is_mid_drawing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) 
  {
    dont_start_drawing_this_frame = true;

    if (G->current_record != 0)
    {
      Draw_record* record                    = G->current_record;
      RenderTexture texture_before_we_affected = record->texture_before_we_affected;
      
      // Change the affected part from the fresh texture to the stored old version
      copy_from_texture_to_texture(
        G->draw_texture_always_fresh.texture, v2u64(0, 0), 
        texture_before_we_affected.texture, v2u64(0, 0), 
        (U64)texture_before_we_affected.texture.width, (U64)texture_before_we_affected.texture.height
      );

      // Change the affected part from the not so fresh texture to the stored old version
      copy_from_texture_to_texture(
        G->draw_texture_not_that_fresh.texture, v2u64(0, 0), 
        texture_before_we_affected.texture, v2u64(0, 0), 
        (U64)texture_before_we_affected.texture.width, (U64)texture_before_we_affected.texture.height
      );
      
      G->current_record = G->current_record->prev;
    }
  }
  else // User want to clear the screen
  if (!G->is_mid_drawing && IsKeyPressed(KEY_DELETE))
  {
    dont_start_drawing_this_frame = true;

    // Creating a new current record
    Draw_record_registration_result record_reg = register_new_draw_record(G, is_ui_capturing_mouse);
    if (record_reg.succ)
    {
      Draw_record* record = record_reg.record;
      G->current_record = record_reg.record;
    
      U64 w = G->draw_texture_always_fresh.texture.width;
      U64 h = G->draw_texture_always_fresh.texture.height;
      
      // todo: These storing of 4 textures might be seprated into their own call
  
      // Clearing the texture
      fill_texture_with_color(G->draw_texture_always_fresh, { 0, 0, 0, 0 }, true);
  
      // Storing the prev texture state
      copy_from_texture_to_texture(
        G->current_record->texture_before_we_affected.texture, v2u64(0, 0),
        G->draw_texture_not_that_fresh.texture, v2u64(0, 0),
        G->draw_texures_width, G->draw_texures_height
      );
  
      // Storing the new texture state
      copy_from_texture_to_texture(
        G->current_record->texture_after_we_affected.texture, v2u64(0, 0),
        G->draw_texture_always_fresh.texture, v2u64(0, 0),
        G->draw_texures_width, G->draw_texures_height
      );
  
      // Matching the prev texture state to the new one
      copy_from_texture_to_texture(
        G->draw_texture_not_that_fresh.texture, v2u64(0, 0),
        G->draw_texture_always_fresh.texture, v2u64(0, 0),
        G->draw_texures_width, G->draw_texures_height
      );
    }

  }
  else // User wants to start using the eraser pen 
  if (!G->is_mid_drawing && IsKeyPressed(KEY_E))
  {
    dont_start_drawing_this_frame = true;
    G->is_erasing_mode = true;
  }
  else // User wants to start using the brush/pen
  if (!G->is_mid_drawing && IsKeyPressed(KEY_B))
  {
    dont_start_drawing_this_frame = true;
    G->is_erasing_mode = false;
  } 
  else
  if (IsKeyPressed(KEY_TAB))
  {
    ToggleBool(G->show_brush_ui_menu);
    ui_reset_active_match(G->brush_menu_ui_id);
  }

  if (dont_start_drawing_this_frame) { goto __active_draw_update_routine_end__; }

  // User is about to start drawing
  if (!G->is_mid_drawing && !is_ui_capturing_mouse && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) 
  {
    Draw_record_registration_result record_registation = register_new_draw_record(G, is_ui_capturing_mouse);
    if (record_registation.succ)
    {
      G->is_mid_drawing = true;
    }
  }
  else // Updating active drawing 
  if (G->is_mid_drawing && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
  {
    Vector2 new_pos = GetMousePosition();
    Vector2 prev_pos = Vector2Subtract(new_pos, GetMouseDelta());
    
    // todo: This is not the best way to have this here like this, but for now thats how it is
    V4U8 color = { 0, 0, 0 ,0 };
    if (!G->is_erasing_mode) { color = G->pen_color; }
    U64 pen_size = G->eraser_size;
    if (!G->is_erasing_mode) { pen_size = G->pen_size; }
    draw_onto_texture(G->draw_texture_always_fresh, new_pos, prev_pos, pen_size, color, G->is_erasing_mode);

    // todo: Update this comment here (We no longer use cpu side image)
    // note: Here is the only time when the cpu side image is not synced to the gpu one. 
    //       But we do later sync it when we stop drawing.
  }
  else // Here we finalise the draw recordd that the user have been drawing
  if (G->is_mid_drawing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
  {
    Assert(G->current_record != 0);
    Assert(G->current_record->texture_after_we_affected.texture.id != 0);  // These are expected to already be allocated by this point
    Assert(G->current_record->texture_before_we_affected.texture.id != 0); // These are expected to already be allocated by this point
    
    G->is_mid_drawing = false;

    U64 draw_t_width = G->draw_texture_always_fresh.texture.width;
    U64 draw_t_height = G->draw_texture_always_fresh.texture.height;

    Draw_record* record = G->current_record;
    RenderTexture* texture_before_we_affected = &record->texture_before_we_affected;
    RenderTexture* texture_after_we_affected  = &record->texture_after_we_affected;
    
    // note: By this point, the fresh draw texture is the new final version of what the user has draw

    // Storing the prev version of the draw texture 
    copy_from_texture_to_texture(
      texture_before_we_affected->texture, v2u64(0, 0),
      G->draw_texture_not_that_fresh.texture, v2u64(0, 0),
      G->draw_texures_width, G->draw_texures_height
    );

    // Storing the new version of the draw texture
    copy_from_texture_to_texture(
      texture_after_we_affected->texture, v2u64(0, 0),
      G->draw_texture_always_fresh.texture, v2u64(0, 0),
      G->draw_texures_width, G->draw_texures_height
    );

    // Updating the prev version of the draw texture to match the new version
    copy_from_texture_to_texture(
      G->draw_texture_not_that_fresh.texture, v2u64(0, 0),
      G->draw_texture_always_fresh.texture, v2u64(0, 0),
      G->draw_texures_width, G->draw_texures_height
    );
  }

  __active_draw_update_routine_end__: {};

}

// todo: Maybe have G be const
void update_pencil_ui(G_state* G, RLI_Event_list* rli_events)
{
  int w = GetScreenWidth();
  int h = GetScreenHeight();
  ui_begin_build((F32)GetScreenWidth(), (F32)GetScreenHeight(), (F32)GetMouseX(), (F32)GetMouseY());

  ui_push_font(G->font_texture_for_ui);

  ui_set_next_border({ 2, { 255, 0, 0, 255 } });
  ui_set_next_size_x(ui_p_of_p(1, 1));
  ui_set_next_size_y(ui_p_of_p(1, 1));
  ui_set_next_layout_axis(Axis2__x);
  UI_Box* top_wrapper = ui_box_make(Str8{}, 0);

  UI_Box* content_inner = 0;
  UI_Parent(top_wrapper)
  {
    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_color({ 255, 0, 0, 255 });
    UI_Box* left_border = ui_box_make(Str8{}, 0);
    
    ui_set_next_size_x(ui_p_of_p(1, 0));
    ui_set_next_size_y(ui_p_of_p(1, 0));
    ui_set_next_layout_axis(Axis2__y);
    UI_Box* content_outer = ui_box_make(Str8{}, 0);

    UI_Parent(content_outer)
    {
      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_color({ 255, 0, 0, 255 });
      UI_Box* top_border = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 0));
      ui_set_next_size_y(ui_p_of_p(1, 0));
      ui_set_next_layout_axis(Axis2__y);
      content_inner = ui_box_make(Str8{}, 0);

      ui_set_next_size_x(ui_p_of_p(1, 1));
      ui_set_next_size_y(ui_px(10));
      ui_set_next_color({ 255, 0, 0, 255 });
      UI_Box* bottom_border = ui_box_make(Str8{}, 0);
    }

    ui_set_next_size_x(ui_px(10));
    ui_set_next_size_y(ui_p_of_p(1, 1));
    ui_set_next_color({ 255, 0, 0, 255 });
    UI_Box* right_border = ui_box_make(Str8{}, 0);
  }

  if (G->show_brush_ui_menu)
  {
    UI_Parent(content_inner)
    {
      ui_set_next_flags(UI_Box_flag__floating);
      ui_set_next_size_x(ui_p_of_p(1, 1)); ui_set_next_size_y(ui_p_of_p(1, 1));
      ui_set_next_layout_axis(Axis2__y);
      UI_Box* brush_wrapper_box_x = ui_box_make(Str8FromC("Wrapper of floating boxes id"), 0);
      
      Str8 brush_floating_hook_box_id = Str8FromC("Brush floating box hook id");
      G->brush_menu_ui_id = brush_floating_hook_box_id;
  
      static F32 drag_mouse_hold_pos_x = 0;
      static F32 drag_mouse_hold_pos_y = 0;
  
      static F32 x_offset = 50.0f;
      static F32 y_offset = 100.0f;
  
      static B32 is_brush_menu_open = false;
  
      F32 x_offset_old = x_offset;
      F32 y_offset_old = y_offset;
  
      UI_Actions brush_box_actions = ui_actions_from_id(brush_floating_hook_box_id, rli_events);
      UI_Box_data brush_box_actions_data = ui_get_box_data_prev_frame(brush_floating_hook_box_id);
      if (!brush_box_actions.was_down && brush_box_actions.is_down)
      {
        ui_set_active(brush_floating_hook_box_id);
        drag_mouse_hold_pos_x = (F32)GetMouseX() - brush_box_actions_data.on_screen_rect.x;
        drag_mouse_hold_pos_y = (F32)GetMouseY() - brush_box_actions_data.on_screen_rect.y;
      }
      else if (brush_box_actions.is_down)
      {
        UI_Box_data float_space_box_data = ui_get_box_data_prev_frame(brush_wrapper_box_x->id);
        x_offset = (F32)GetMouseX() - drag_mouse_hold_pos_x - float_space_box_data.on_screen_rect.x;
        y_offset = (F32)GetMouseY() - drag_mouse_hold_pos_y - float_space_box_data.on_screen_rect.y;
      }
      else
      {
        ui_reset_active_match(brush_floating_hook_box_id);
        drag_mouse_hold_pos_x = 0;
        drag_mouse_hold_pos_y = 0;
      }
  
      UI_Parent(brush_wrapper_box_x)
      {
        ui_spacer(ui_px(y_offset_old));
  
        ui_set_next_size_x(ui_p_of_p(1, 1)); ui_set_next_size_y(ui_p_of_p(1, 1));
        ui_set_next_layout_axis(Axis2__x);
        UI_Box* brush_wrapper_box_y = ui_box_make({}, 0);
        UI_Parent(brush_wrapper_box_y)
        {
          ui_spacer(ui_px(x_offset_old));
  
          ui_set_next_size_x(ui_px(200)); ui_set_next_size_y(ui_children_sum());
          ui_set_next_layout_axis(Axis2__y);
          // ui_set_next_corner_radius(0.15f);
          ui_set_next_color({ 255, 255, 255, 255 });
          UI_Box* brushes_menu_box = ui_box_make({}, 0);
          UI_Parent(brushes_menu_box)
          {
            // Header hook box
            ui_set_next_size_x(ui_p_of_p(1, 0)); ui_set_next_size_y(ui_children_sum());
            ui_set_next_layout_axis(Axis2__y);
            ui_set_next_color({ 151, 184, 210, 255 });
            UI_Box* hook_box = ui_box_make(brush_floating_hook_box_id, 0);
            UI_Parent(hook_box)
            {
              ui_spacer(ui_px(5));
  
              UI_PaddedBox(ui_px(5), Axis2__x)
              {
                ui_set_next_text_color({ 0, 0, 0, 255 });
                ui_set_next_font_size(24);
                ui_label_c("Brushes");
  
                ui_set_next_color({ 255, 0, 0, 255 });
                
              }
  
              ui_spacer(ui_px(5));
            }
    
            UI_XStack()
            {
              ui_spacer(ui_p_of_p(1, 0));
  
              UI_PaddedBox(ui_px(5), Axis2__y) 
              {
                ui_spacer(ui_px(10));
      
                if (G->is_erasing_mode)
                {
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style size_slider_style = {};
                    size_slider_style.height         = 20;
                    size_slider_style.width          = 100;
                    size_slider_style.hover_color    = { 169, 205, 246, 255 };
                    size_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    size_slider_style.text_color     = { 0, 0, 0, 255 };
                    size_slider_style.font_size      = 20;
                    size_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    size_slider_style.fmt_str = "%.0f";
        
                    U64 new_eraser_size = (U64)ui_slider(Str8FromC("Eraser size slider id"), &size_slider_style, (F32)G->eraser_size, 1, 100, rli_events);
                    if (new_eraser_size != G->eraser_size)
                    {
                      G->signal_new_eraser_size = true;
                      G->new_eraser_size = new_eraser_size;
                    } 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Size");
                  }
        
                  ui_spacer(ui_px(10));
    
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    ui_set_next_color({ 175, 203, 255, 255 });
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    UI_PaddedBox(ui_px(5), Axis2__x)
                    {
                      UI_Actions pen_button = ui_button(Str8FromC("Brush - [B]## button"), rli_events);
                      if (pen_button.is_clicked)
                      {
                        G->signal_swap_to_pen= true;
                      }
                    }
                  }
                }
                else
                {
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style size_slider_style = {};
                    size_slider_style.height         = 20;
                    size_slider_style.width          = 100;
                    size_slider_style.hover_color    = { 169, 205, 246, 255 };
                    size_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    size_slider_style.text_color     = { 0, 0, 0, 255 };
                    size_slider_style.font_size      = 20;
                    size_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    size_slider_style.fmt_str = "%.0f";
        
                    U64 new_pen_size = (U64)ui_slider(Str8FromC("Pen size slider id"), &size_slider_style, (F32)G->pen_size, 1, 100, rli_events);
                    clamp_u64_inplace(&new_pen_size, 1, 100);
                    if (new_pen_size != G->pen_size)
                    {
                      G->signal_new_pen_size = true;
                      G->new_pen_size = new_pen_size;
                    } 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Size");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style red_slider_style = {};
                    red_slider_style.height         = 20;
                    red_slider_style.width          = 100;
                    red_slider_style.hover_color    = { 169, 205, 246, 255 };
                    red_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    red_slider_style.text_color     = { 0, 0, 0, 255 };
                    red_slider_style.font_size      = 20;
                    red_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    red_slider_style.fmt_str = "%.0f";
        
                    U8 new_r_color = (U8)ui_slider(Str8FromC("Red slider style id"), &red_slider_style, (F32)G->pen_color.r, 0, 255, rli_events);
                    G->pen_color.r = new_r_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Red");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style green_slider_style = {};
                    green_slider_style.height         = 20;
                    green_slider_style.width          = 100;
                    green_slider_style.hover_color    = { 169, 205, 246, 255 };
                    green_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    green_slider_style.text_color     = { 0, 0, 0, 255 };
                    green_slider_style.font_size      = 20;
                    green_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    green_slider_style.fmt_str = "%.0f";
        
                    U8 new_g_color = (U8)ui_slider(Str8FromC("Green slider style id"), &green_slider_style, (F32)G->pen_color.g, 0, 255, rli_events);
                    G->pen_color.g = new_g_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Green");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style blue_slider_style = {};
                    blue_slider_style.height         = 20;
                    blue_slider_style.width          = 100;
                    blue_slider_style.hover_color    = { 169, 205, 246, 255 };
                    blue_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    blue_slider_style.text_color     = { 0, 0, 0, 255 };
                    blue_slider_style.font_size      = 20;
                    blue_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    blue_slider_style.fmt_str = "%.0f";
        
                    U8 new_b_color = (U8)ui_slider(Str8FromC("Blue slider style id"), &blue_slider_style, (F32)G->pen_color.b, 0, 255, rli_events);
                    G->pen_color.b = new_b_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Blue");
                  }
        
                  ui_spacer(ui_px(2));
        
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    UI_Slider_style alpa_slider_style = {};
                    alpa_slider_style.height         = 20;
                    alpa_slider_style.width          = 100;
                    alpa_slider_style.hover_color    = { 169, 205, 246, 255 };
                    alpa_slider_style.no_hover_color = { 220, 220, 220, 255 };
                    alpa_slider_style.text_color     = { 0, 0, 0, 255 };
                    alpa_slider_style.font_size      = 20;
                    alpa_slider_style.slided_part_color = { 97, 171, 255, 255 };
                    alpa_slider_style.fmt_str = "%.0f";
        
                    U8 new_a_color = (U8)ui_slider(Str8FromC("Alpha slider style id"), &alpa_slider_style, (F32)G->pen_color.a, 0, 255, rli_events);
                    G->pen_color.a = new_a_color; 
        
                    ui_spacer(ui_px(10));
        
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    ui_label_c("Alpha");
                  }
                  
                  ui_spacer(ui_px(10));
          
                  UI_PaddedBox(ui_px(5), Axis2__x)
                  {
                    ui_set_next_color({ 175, 203, 255, 255 });
                    ui_set_next_text_color({ 0, 0, 0, 255 });
                    ui_set_next_font_size(20);
                    UI_PaddedBox(ui_px(5), Axis2__x)
                    {
                      UI_Actions eraser_button = ui_button(Str8FromC("Eraser - [E]## button"), rli_events);
                      if (eraser_button.is_clicked)
                      {
                        G->signal_swap_to_eraser= true;
                      }
                    }
                  }
                }
              }
  
              ui_spacer(ui_p_of_p(1, 0));
            }
  
          }
        }
      }
    }
  }

  ui_end_build();
}


WNDPROC raylib_winproc = 0;

B32 hot_key_activated = false;

LRESULT custom_win_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
  Assert(raylib_winproc != 0);

  LRESULT result = {};
  if (message == WM_HOTKEY) {
    hot_key_activated = true;
    result = TRUE;
  } 
  else {
    result = CallWindowProc(raylib_winproc, window_handle, message, w_param, l_param);
  }
  return result;
}

int main()
{
  os_init();
  allocate_thread_context();
  os_win32_set_thread_context(get_thread_context()); // todo: This is a flaw of the os system and the fact that we link in
  RLI_init(os_get_keyboard_initial_repeat_delay(), os_get_keyboard_subsequent_repeat_delay());
  ui_init();

  ui_set_text_measuring_function(ui_text_measure_func);
  
  // FLAG_WINDOW_TOPMOST
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT);
  InitWindow(800, 600, "Pencil");

  // HideCursor();

  Image logo = LoadImage("../data/logo.png");
  HandleLater(logo.format == PixelFormat_UNCOMPRESSED_R8G8B8A8);
  SetWindowIcon(logo);
  // Setting the custom win32 winproc to handle registered system shortcuts
  HWND win32_window_handle = (HWND)GetWindowHandle();
  raylib_winproc = (WNDPROC)GetWindowLongPtrA(win32_window_handle, GWLP_WNDPROC);
  Assert(raylib_winproc != 0);
  LONG_PTR set_succ_proc = SetWindowLongPtrA(win32_window_handle, GWLP_WNDPROC, (LONG_PTR)custom_win_proc);
  Assert(set_succ_proc != 0);

  // todo:
  LONG_PTR set_succ_style = SetWindowLongPtrA(win32_window_handle, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
  Assert(set_succ_style != 0);
  
  BOOL hot_key_register_succ = RegisterHotKey((HWND)GetWindowHandle(), 69, MOD_SHIFT|MOD_ALT, 'S');

  SetExitKey(KEY_NULL);

  SetWindowState(FLAG_WINDOW_TOPMOST);
  SetWindowState(FLAG_WINDOW_UNDECORATED);
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetWindowState(FLAG_WINDOW_MAXIMIZED);

  glCopyImageSubData = (glCopyImageSubData_fp)wglGetProcAddress("glCopyImageSubData");
  HandleLater(glCopyImageSubData != (void*)0 && glCopyImageSubData != (void*)1 && glCopyImageSubData != (void*)2 && glCopyImageSubData != (void*)3 && glCopyImageSubData != (void*)-1);

  G_state G = {};

  G.arena = arena_alloc(Megabytes(64));
  G.frame_arena = arena_alloc(Megabytes(64));
  
  G.pen_size = 11;
  G.pen_color = v4u8(255, 255, 255, 255);

  G.eraser_size = G.pen_size * 2;

  G.draw_texures_width = (U64)GetScreenWidth();
  G.draw_texures_height = (U64)GetScreenHeight();

  G.draw_texture_always_fresh = LoadRenderTexture((int)G.draw_texures_width, (int)G.draw_texures_height); 
  Assert(G.draw_texture_always_fresh.id != 0);
  BeginTextureMode(G.draw_texture_always_fresh); { ClearBackground(BLANK); } EndTextureMode(); 

  G.draw_texture_not_that_fresh = LoadRenderTexture((int)G.draw_texures_width, (int)G.draw_texures_height); 
  Assert(G.draw_texture_not_that_fresh.id != 0);
  BeginTextureMode(G.draw_texture_not_that_fresh); { ClearBackground(BLANK); } EndTextureMode(); 

  G.font_texture_for_ui = LoadFont("../data/Roboto.ttf");
  G.last_screen_dims = {};
  {
    int w = GetScreenWidth();  Assert(w >= 0);
    int h = GetScreenHeight(); Assert(h >= 0);
    G.last_screen_dims = v2u64((U64)w, (U64)h);
  }
  
  for (;!WindowShouldClose();)
  {
    // if (IsKeyPressed(KEY_ESCAPE)) { return 0; }

    // todo: 
    // There shoud be a good way to just stop doing what the user is doing 
    // in the update loop to not have to deal with braking bugs.
    // This way the logic might be wrong an the thing wont work as intended,
    // but it wont be staight up broken, but rather just have a default
    // behaviour.      
    B32 are_we_interactive = !IsWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);

    if (hot_key_activated)
    {
      hot_key_activated = false;
      ToggleBool(are_we_interactive);
    }

    if (are_we_interactive) { 
      ClearWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH); 
      // SetWindowFocused();      
    }
    else { 
      SetWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
      // SetWindowState(FLAG_WINDOW_UNFOCUSED);
    } 

    // Chaning the cursor
    // todo: Not sure how much i like to have these here like that, but for now its here
    if (are_we_interactive) {
      if (G.is_erasing_mode) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
      } else {
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);  
      }
    } else {
      SetMouseCursor(MOUSE_CURSOR_DEFAULT);  
    }

    // note: These were the attemps to fix the task bar movement issue for window sizing, didnt work
    // RECT rect = {};
    // BOOL succ = SystemParametersInfoA(SPI_GETWORKAREA, 0, &rect, 0);
    // Assert(succ);
    // int x = 0;
    // SetWindowSize(rect.right - rect.left, rect.bottom - rect.top);
    // Assert(GetScreenWidth() == rect.right - rect.left);
    // Assert(GetScreenHeight() == rect.bottom- rect.top);
    // getclientrect
    // MaximizeWindow();
    // RECT rect = {};
    // HWND window_handle = (HWND)GetWindowHandle();
    // BOOL succ = GetClientRect(window_handle, &rect);

    if (are_we_interactive && IsKeyPressed(KEY_P))
    {
      _debug_load_gpu_textures_onto_cpu(&G);
    }

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
    
    // todo: If ui start to have some animations, we will have to not just not update it, but rather 
    //       just not take inputs but still update all the rest of the things
    
    
    if (are_we_interactive)
    {
      SetWindowFocused();
      if (!G.is_mid_drawing) {
        update_pencil_ui(&G, rli_events);
      }
  
      update_pencil(&G, ui_has_active());
    }

    DeferLoop(BeginDrawing(), EndDrawing())
    {
      ClearBackground(BLANK);
      
      DrawTexture(G.draw_texture_always_fresh.texture, 0, 0, WHITE);
      
      if (are_we_interactive) { ui_draw(); }
      
      // Cursor
      // if (are_we_interactive)
      // {
      //   if (ui_has_active())
      //   {
      //     ShowCursor();
      //   }
      //   else 
      //   {
      //     ShowCursor();
      //     // V4U8 color = G.pen_color;
      //     // color.r = ~color.r;
      //     // color.g = ~color.g;
      //     // color.b = ~color.b;
      //     DrawCircleLinesV(GetMousePosition(), (F32)G.pen_size, { color.r, color.g, color.b, color.a });
      //   } 
      // }

      DrawFPS(0, 0);
    }
  }
  
  // note: not releasing stuff, defering it to the system
  return 0;
}


// int main(void)
// {
//     // Initialization
//     //---------------------------------------------------------
//     const int screenWidth = 800;
//     const int screenHeight = 450;

//     // Possible window flags
//     /*
//     FLAG_VSYNC_HINT
//     FLAG_FULLSCREEN_MODE    -> not working properly -> wrong scaling!
//     FLAG_WINDOW_RESIZABLE
//     FLAG_WINDOW_UNDECORATED
//     FLAG_WINDOW_TRANSPARENT
//     FLAG_WINDOW_HIDDEN
//     FLAG_WINDOW_MINIMIZED   -> Not supported on window creation
//     FLAG_WINDOW_MAXIMIZED   -> Not supported on window creation
//     FLAG_WINDOW_UNFOCUSED
//     FLAG_WINDOW_TOPMOST
//     FLAG_WINDOW_HIGHDPI     -> errors after minimize-resize, fb size is recalculated
//     FLAG_WINDOW_ALWAYS_RUN
//     FLAG_MSAA_4X_HINT
//     */

//     // Set configuration flags for window creation
//     SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
//     // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);// | FLAG_WINDOW_TRANSPARENT);
//     InitWindow(screenWidth, screenHeight, "raylib [core] example - window flags");

//     Vector2 ballPosition = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
//     Vector2 ballSpeed = { 5.0f, 4.0f };
//     float ballRadius = 20;

//     int framesCounter = 0;

//     SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
//     //----------------------------------------------------------

//     // Main game loop
//     while (!WindowShouldClose())    // Detect window close button or ESC key
//     {
//         // Update
//         //-----------------------------------------------------
//         if (IsKeyPressed(KEY_F)) ToggleFullscreen();  // modifies window size when scaling!

//         if (IsKeyPressed(KEY_R))
//         {
//             if (IsWindowState(FLAG_WINDOW_RESIZABLE)) ClearWindowState(FLAG_WINDOW_RESIZABLE);
//             else SetWindowState(FLAG_WINDOW_RESIZABLE);
//         }

//         if (IsKeyPressed(KEY_D))
//         {
//             if (IsWindowState(FLAG_WINDOW_UNDECORATED)) ClearWindowState(FLAG_WINDOW_UNDECORATED);
//             else SetWindowState(FLAG_WINDOW_UNDECORATED);
//         }

//         if (IsKeyPressed(KEY_H))
//         {
//             if (!IsWindowState(FLAG_WINDOW_HIDDEN)) SetWindowState(FLAG_WINDOW_HIDDEN);

//             framesCounter = 0;
//         }

//         if (IsWindowState(FLAG_WINDOW_HIDDEN))
//         {
//             framesCounter++;
//             if (framesCounter >= 240) ClearWindowState(FLAG_WINDOW_HIDDEN); // Show window after 3 seconds
//         }

//         if (IsKeyPressed(KEY_N))
//         {
//             if (!IsWindowState(FLAG_WINDOW_MINIMIZED)) MinimizeWindow();

//             framesCounter = 0;
//         }

//         if (IsWindowState(FLAG_WINDOW_MINIMIZED))
//         {
//             framesCounter++;
//             if (framesCounter >= 240)
//             {
//                 RestoreWindow(); // Restore window after 3 seconds
//                 framesCounter = 0;
//             }
//         }

//         if (IsKeyPressed(KEY_M))
//         {
//             // NOTE: Requires FLAG_WINDOW_RESIZABLE enabled!
//             if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) RestoreWindow();
//             else MaximizeWindow();
//         }

//         if (IsKeyPressed(KEY_U))
//         {
//             if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) ClearWindowState(FLAG_WINDOW_UNFOCUSED);
//             else SetWindowState(FLAG_WINDOW_UNFOCUSED);
//         }

//         if (IsKeyPressed(KEY_T))
//         {
//             if (IsWindowState(FLAG_WINDOW_TOPMOST)) ClearWindowState(FLAG_WINDOW_TOPMOST);
//             else SetWindowState(FLAG_WINDOW_TOPMOST);
//         }

//         if (IsKeyPressed(KEY_A))
//         {
//             if (IsWindowState(FLAG_WINDOW_ALWAYS_RUN)) ClearWindowState(FLAG_WINDOW_ALWAYS_RUN);
//             else SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
//         }

//         if (IsKeyPressed(KEY_V))
//         {
//             if (IsWindowState(FLAG_VSYNC_HINT)) ClearWindowState(FLAG_VSYNC_HINT);
//             else SetWindowState(FLAG_VSYNC_HINT);
//         }

//         if (IsKeyPressed(KEY_B)) ToggleBorderlessWindowed();


//         // Bouncing ball logic
//         ballPosition.x += ballSpeed.x;
//         ballPosition.y += ballSpeed.y;
//         if ((ballPosition.x >= (GetScreenWidth() - ballRadius)) || (ballPosition.x <= ballRadius)) ballSpeed.x *= -1.0f;
//         if ((ballPosition.y >= (GetScreenHeight() - ballRadius)) || (ballPosition.y <= ballRadius)) ballSpeed.y *= -1.0f;
//         //-----------------------------------------------------

//         // Draw
//         //-----------------------------------------------------
//         BeginDrawing();

//         if (IsWindowState(FLAG_WINDOW_TRANSPARENT)) ClearBackground(BLANK);
//         else ClearBackground(RAYWHITE);

//         DrawCircleV(ballPosition, ballRadius, MAROON);
//         DrawRectangleLinesEx({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, 4, RAYWHITE);

//         DrawCircleV(GetMousePosition(), 10, DARKBLUE);

//         DrawFPS(10, 10);

//         DrawText(TextFormat("Screen Size: [%i, %i]", GetScreenWidth(), GetScreenHeight()), 10, 40, 10, GREEN);

//         // Draw window state info
//         DrawText("Following flags can be set after window creation:", 10, 60, 10, GRAY);
//         if (IsWindowState(FLAG_FULLSCREEN_MODE)) DrawText("[F] FLAG_FULLSCREEN_MODE: on", 10, 80, 10, LIME);
//         else DrawText("[F] FLAG_FULLSCREEN_MODE: off", 10, 80, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_RESIZABLE)) DrawText("[R] FLAG_WINDOW_RESIZABLE: on", 10, 100, 10, LIME);
//         else DrawText("[R] FLAG_WINDOW_RESIZABLE: off", 10, 100, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_UNDECORATED)) DrawText("[D] FLAG_WINDOW_UNDECORATED: on", 10, 120, 10, LIME);
//         else DrawText("[D] FLAG_WINDOW_UNDECORATED: off", 10, 120, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_HIDDEN)) DrawText("[H] FLAG_WINDOW_HIDDEN: on", 10, 140, 10, LIME);
//         else DrawText("[H] FLAG_WINDOW_HIDDEN: off (hides for 3 seconds)", 10, 140, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_MINIMIZED)) DrawText("[N] FLAG_WINDOW_MINIMIZED: on", 10, 160, 10, LIME);
//         else DrawText("[N] FLAG_WINDOW_MINIMIZED: off (restores after 3 seconds)", 10, 160, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) DrawText("[M] FLAG_WINDOW_MAXIMIZED: on", 10, 180, 10, LIME);
//         else DrawText("[M] FLAG_WINDOW_MAXIMIZED: off", 10, 180, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) DrawText("[G] FLAG_WINDOW_UNFOCUSED: on", 10, 200, 10, LIME);
//         else DrawText("[U] FLAG_WINDOW_UNFOCUSED: off", 10, 200, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_TOPMOST)) DrawText("[T] FLAG_WINDOW_TOPMOST: on", 10, 220, 10, LIME);
//         else DrawText("[T] FLAG_WINDOW_TOPMOST: off", 10, 220, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_ALWAYS_RUN)) DrawText("[A] FLAG_WINDOW_ALWAYS_RUN: on", 10, 240, 10, LIME);
//         else DrawText("[A] FLAG_WINDOW_ALWAYS_RUN: off", 10, 240, 10, MAROON);
//         if (IsWindowState(FLAG_VSYNC_HINT)) DrawText("[V] FLAG_VSYNC_HINT: on", 10, 260, 10, LIME);
//         else DrawText("[V] FLAG_VSYNC_HINT: off", 10, 260, 10, MAROON);
//         if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) DrawText("[B] FLAG_BORDERLESS_WINDOWED_MODE: on", 10, 280, 10, LIME);
//         else DrawText("[B] FLAG_BORDERLESS_WINDOWED_MODE: off", 10, 280, 10, MAROON);

//         DrawText("Following flags can only be set before window creation:", 10, 320, 10, GRAY);
//         if (IsWindowState(FLAG_WINDOW_HIGHDPI)) DrawText("FLAG_WINDOW_HIGHDPI: on", 10, 340, 10, LIME);
//         else DrawText("FLAG_WINDOW_HIGHDPI: off", 10, 340, 10, MAROON);
//         if (IsWindowState(FLAG_WINDOW_TRANSPARENT)) DrawText("FLAG_WINDOW_TRANSPARENT: on", 10, 360, 10, LIME);
//         else DrawText("FLAG_WINDOW_TRANSPARENT: off", 10, 360, 10, MAROON);
//         if (IsWindowState(FLAG_MSAA_4X_HINT)) DrawText("FLAG_MSAA_4X_HINT: on", 10, 380, 10, LIME);
//         else DrawText("FLAG_MSAA_4X_HINT: off", 10, 380, 10, MAROON);

//         EndDrawing();
//         //-----------------------------------------------------
//     }

//     // De-Initialization
//     //---------------------------------------------------------
//     CloseWindow();        // Close window and OpenGL context
//     //----------------------------------------------------------

//     return 0;
// }