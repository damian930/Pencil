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

// note: This is for hooks, which i dont yet use
// #include "dwmapi.h"
// #pragma comment(lib, "dwmapi.lib")

struct Node_with_points {
  // B32 is_active;
  // Node_with_points* next;
  V2 points[100];
  U16 points_count;
};
StaticAssert(ArrayCount(Node_with_points::points) <= u16_max);

struct Draw_record {
  Node_with_points* node_with_points;
};

struct G_state {
  Arena* frame_arena;
  Texture draw_texture;
  U64 pen_size;
  V4 pen_color;


  // Draw_record draw_record;
  Arena* arena_for_nodes_with_points;
  U64 nodes_with_points_count;
  Node_with_points* first_node_with_points;
  
  // note: This is for later 
  // Draw_record draw_reconds[10];

  B32 is_mid_drawing;
};

// Ring buffer or draw recconds
// Each reccond is a line, a line is the thing we drew while holding the left mouse button down 

V2 ui_text_measure_func(Str8 text, Font font, F32 font_size)
{ 
  Scratch scratch = get_scratch(0, 0);
  Str8 text_nt = str8_copy_alloc(scratch.arena, text);
  Vector2 rl_vec = MeasureTextEx(font, (char*)text_nt.data, font_size, 0);
  V2 result = { rl_vec.x, rl_vec.y };
  end_scratch(&scratch);
  return result;
}

Node_with_points* get_current_node_with_points(const G_state* G)
{
  Assert(G->nodes_with_points_count > 0);
  Node_with_points* node = G->first_node_with_points + (G->nodes_with_points_count - 1);
  return node;
}

void store_point(G_state* G, V2 point)
{
  Node_with_points* current_node_with_points = get_current_node_with_points(G);

  if (current_node_with_points->points_count == ArrayCount(current_node_with_points->points))
  {
    ArenaPush(G->arena_for_nodes_with_points, Node_with_points);
    G->nodes_with_points_count += 1;
    current_node_with_points = get_current_node_with_points(G);
  }

  V2* new_point = current_node_with_points->points + (current_node_with_points->points_count++);
  *new_point = point;
}

void update_pencil(G_state* G)
{
  static B32 done_with_the_first_draw = false;
  if (G->is_mid_drawing && IsMouseButtonUp(MOUSE_BUTTON_LEFT))
  {
    done_with_the_first_draw = true;
  }

  // note: for now we can only draw once
  if (done_with_the_first_draw == false && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
  {
    // Setting up the new Draw_record to be filled
    if (!G->is_mid_drawing) 
    { 
      G->is_mid_drawing = true;
      // G->draw_record = Draw_record{};
      Assert(G->first_node_with_points == ArenaCurrentPos(G->arena_for_nodes_with_points, Node_with_points));
      G->nodes_with_points_count = 0;
      arena_clear(G->arena_for_nodes_with_points);
      
      ArenaPush(G->arena_for_nodes_with_points, Node_with_points);
      G->nodes_with_points_count += 1;
    }
    
    if (G->is_mid_drawing)
    {
      Vector2 mouse_pos = GetMousePosition();
  
      // Updating the current Draw_record we are creating
      Assert(G->nodes_with_points_count > 0);
      Node_with_points* current_node = get_current_node_with_points(G);
      if (current_node->points_count == 0) // 
      {
        store_point(G, { mouse_pos.x, mouse_pos.y }); // Just store 
      }
      else 
      {
        // Get the last one, see if it is the same as the possible new one and if it is, then dont store the new one
        V2 last_stored_point = current_node->points[current_node->points_count - 1];
        if (last_stored_point.x != mouse_pos.x || last_stored_point.y != mouse_pos.y)
        {
          store_point(G, { mouse_pos.x, mouse_pos.y });
        }
      }
  
      // Updating the drawing texture on the GPU
      Temp_arena temp = temp_arena_begin(G->frame_arena);
      U32* px_to_update = ArenaPush(temp.arena, U32);
      ((U8*)(px_to_update))[0] = G->pen_color.r;
      ((U8*)(px_to_update))[1] = G->pen_color.g;
      ((U8*)(px_to_update))[2] = G->pen_color.b;
      ((U8*)(px_to_update))[3] = G->pen_color.a;
      Rectangle affected_rect = { mouse_pos.x, mouse_pos.y, 1, 1 };
      UpdateTextureRec(G->draw_texture, affected_rect, px_to_update);
      temp_arena_end(&temp);
    }
  } 
  else if (IsKeyDown(KEY_BACKSPACE))
  {
    // Remove the last Draw_recond from the draw_texture
    for (U64 node_index = 0; node_index < G->nodes_with_points_count; node_index += 1)
    {
      Node_with_points* node = G->first_node_with_points + node_index;
      for (U64 point_index = 0; point_index < node->points_count; point_index += 1)
      {
        V2 point = node->points[point_index];
        U32 px_to_remove = 0;
        ((U8*)(&px_to_remove))[3] = 255;
        Rectangle update_rect = { point.x, point.y, 1, 1 };
        UpdateTextureRec(G->draw_texture, update_rect, &px_to_remove);
      }
    }

    G->nodes_with_points_count = 0;
    G->first_node_with_points = ArenaCurrentPos(G->arena_for_nodes_with_points, Node_with_points);
    arena_clear(G->arena_for_nodes_with_points);
  }
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
  Font font_roboto = LoadFont("../data/Roboto.ttf");

  G_state G = {};
  G.frame_arena = arena_alloc(Megabytes(64));
  G.arena_for_nodes_with_points = arena_alloc(Megabytes(64));
  G.first_node_with_points = ArenaCurrentPos(G.arena_for_nodes_with_points, Node_with_points);
  G.pen_size = 1;
  G.pen_color = { 0, 0, 255, 255 };

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

  for (;!WindowShouldClose();)
  {
    arena_clear(G.frame_arena);
    RLI_Event_list* rli_events = RLI_get_frame_inputs(); 

    update_pencil(&G);
   
    /*
    DeferLoop(ui_begin_build((F32)GetScreenWidth(), (F32)GetScreenHeight(), (F32)GetMouseX(), (F32)GetMouseY()), ui_end_build())
    {
      ui_push_font(font_roboto);

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
            static U64 size = 1; 
            size = (U64)ui_slider(Str8FromC("Size slider id"), &size_slider_style, (F32)size, 1, 100, rli_events);

            ui_spacer(ui_px(25));

            ui_set_next_color({ 107, 75, 10, 255 });
            UI_PaddedBox(ui_px(5), Axis2__x)
            {
              Str8 edit_box_id = Str8FromC("Edit box for red color"); 
              static UI_Text_edit_box_state edit_box_state = {}; 
              static UI_Size edit_box_size_x = ui_px(100); 
              static U8 text_buffer[2] = "0"; 
              static U64 buffer_max_size = ArrayCount(text_buffer); 
              static U64 buffer_current_size = 1; 
              static U64 cursor_pos = 0;
              static U64 section_start_pos = 0;
              ui_text_box_mutates(edit_box_id, Str8{}, &edit_box_state, edit_box_size_x, text_buffer, buffer_max_size, &buffer_current_size, &cursor_pos, &section_start_pos, rli_events);
            }

            // add: in place text edit field editing

          }

          ui_spacer(ui_px(25));
          ui_spacer(ui_p_of_p(1, 0));
        }
        ui_spacer(ui_px(10));
      }
    }
    */

    DeferLoop(BeginDrawing(), EndDrawing())
    {
      Texture draw_texture = G.draw_texture;
      ClearBackground(BLACK);
      DrawTexturePro(draw_texture, Rectangle{0, 0, (F32)draw_texture.width, (F32)draw_texture.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);   
      DrawFPS(0, 0);
      // ui_draw();
    }
  }


  // note: not releasing stuff, defering it to the system
  return 0;
}