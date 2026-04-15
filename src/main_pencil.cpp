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

struct Node_with_points {
  V2 points[100];
  U16 points_count;
  Node_with_points* next; // This is shared with the free list 
  Node_with_points* prev; 

  // Free list part
  B32 _is_active;
};
StaticAssert(ArrayCount(Node_with_points::points) <= u16_max);

struct Draw_record {
  U64 pen_size;
  Node_with_points* first_node_with_points;
  Node_with_points* last_node_with_points;
  U64 nodes_count;
  
  Draw_record* prev;
  
  // Free list part
  Draw_record* _next_free;
  B32 _is_active;
};

struct G_state {
  Arena* frame_arena;
  Texture draw_texture;
  U64 pen_size;
  V4 pen_color;

  // Pool of draw_records  
  Arena* arena_for_draw_records;
  Draw_record* first_draw_record;
  U64 allocated_draw_records_count;
  Draw_record* first_free_draw_record;

  // Pool of nodes_with_points
  Arena* arena_for_nodes_with_points;
  Node_with_points* first_node_with_points;
  U64 allocated_nodes_with_points_count;
  Node_with_points* first_free_node_with_points;

  // Stuff for while drawing
  B32 is_mid_drawing;
  Draw_record* current_draw_record;

  // Signals (These are just here like this right now)
  B32 signal_new_pen_size;
  U64 new_pen_size;

  // Misc
  Font font_texture_for_ui;
};

// Ring buffer or draw recconds
// Each reccond is a line, a line is the thing we drew while holding the left mouse button down 

/*
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
*/

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
  node->_is_active = true;
  return node;
}

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
  draw_record->_is_active = true;
  return draw_record;
}

void update_pencil(G_state* G, B32 is_ui_capturing_mouse)
{
  Assert(G->first_draw_record != 0);
  Assert(G->first_node_with_points != 0);
  
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

      Node_with_points* new_node = get_available_node_of_points_from_pool(G); // These nodes are just lists, so there is nothing to set up for them to work after getting the pool memory for them
      G->current_draw_record->first_node_with_points = new_node; 
      G->current_draw_record->last_node_with_points  = new_node; 
      G->current_draw_record->nodes_count = 0; 
    }

    if (G->is_mid_drawing) 
    {
      Vector2 mouse_pos = GetMousePosition();
  
      // note: Count of a linked to node cant be zero
      Assert(G->pen_size != 0 && G->pen_size % 2 == 1); // Just making sure that it is an odd value
      Assert(G->current_draw_record && G->current_draw_record->_is_active);
      Assert(G->current_draw_record->first_node_with_points != 0);
      Assert(G->current_draw_record->last_node_with_points != 0);
      
      Node_with_points* last_node = G->current_draw_record->last_node_with_points;
      B32 prev_point_exists = false;
      V2 prev_point = {};
      
      // Adding a new node up front if the last one is full
      if (last_node->points_count == ArrayCount(last_node->points))
      {
        Node_with_points* new_node = get_available_node_of_points_from_pool(G);
        DllPushBack_Name(G->current_draw_record, new_node, first_node_with_points, last_node_with_points, next, prev);
        G->current_draw_record->nodes_count += 1;
        last_node = new_node;
      }
      
      // Checking if the last point exists 
      if (last_node->points_count == 0 && last_node->prev != 0)
      {
        Assert(last_node->prev->points_count == ArrayCount(last_node->prev->points), "Why the fuck do you have a 0 filled node that has an unfilled prev node ???"); 
        prev_point = last_node->prev->points[last_node->prev->points_count - 1];
        prev_point_exists = true;
      }
      else 
      {
        prev_point = last_node->points[last_node->points_count - 1];
        prev_point_exists = true;
      }
  
      // Storing the new points
      B32 shoud_draw_point = false;
      if (!prev_point_exists || mouse_pos.x != prev_point.x || mouse_pos.y != prev_point.y)
      {
        Assert(last_node->points_count != ArrayCount(last_node->points)); // Just checking
        V2* point = last_node->points + (last_node->points_count++);
        point->x = mouse_pos.x;
        point->y = mouse_pos.y;
        shoud_draw_point = true;
      }
  
      if (shoud_draw_point)
      {
        Assert(G->pen_size % 2 == 1); // Hard to draw pen_size, which is the diameter when we press on the middle px, but there is not middle pixel, since the diameter is an even value
  
        Temp_arena temp = temp_arena_begin(G->frame_arena);
        U32* pixels_to_update = ArenaPushArr(temp.arena, U32, G->pen_size * G->pen_size);
        for (U64 col_index = 0; col_index < G->pen_size; col_index += 1)
        {
          for (U64 row_index = 0; row_index < G->pen_size; row_index += 1)
          {
            U32* px = pixels_to_update + (G->pen_size * row_index + col_index);
            ((U8*)(px))[0] = 255; // (U8)G->pen_color.r;
            ((U8*)(px))[1] = 50;  // (U8)G->pen_color.g;
            ((U8*)(px))[2] = 255; // (U8)G->pen_color.b;
            ((U8*)(px))[3] = 255; // (U8)G->pen_color.a; 
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

      // Removing the record from draw_texure
      for (Node_with_points* node = G->current_draw_record->first_node_with_points; node; node = node->next)
      {
        for (U64 p_index = 0; p_index < node->points_count; p_index += 1)
        {
          U64 pen_size = G->current_draw_record->pen_size;
          Temp_arena temp = temp_arena_begin(G->frame_arena);
          U32* pixels_to_update = ArenaPushArr(temp.arena, U32, pen_size * pen_size);
          for (U64 col_index = 0; col_index < pen_size; col_index += 1)
          {
            for (U64 row_index = 0; row_index < pen_size; row_index += 1)
            {
              U32* px = pixels_to_update + (pen_size * row_index + col_index);
              ((U8*)(px))[0] = 0; 
              ((U8*)(px))[1] = 0;  
              ((U8*)(px))[2] = 0; 
              ((U8*)(px))[3] = 0; 
            }
          }
          U64 pixels_on_each_side_to_the_middle_pixel = (U64)(pen_size / 2);
          Rectangle affected_rect = {};
          affected_rect.x      = node->points[p_index].x - pixels_on_each_side_to_the_middle_pixel;
          affected_rect.y      = node->points[p_index].y - pixels_on_each_side_to_the_middle_pixel;
          affected_rect.width  = (F32)pen_size;
          affected_rect.height = (F32)pen_size;
          UpdateTextureRec(G->draw_texture, affected_rect, pixels_to_update);
          temp_arena_end(&temp);
        }
      }

      // Puting the used point nodes by the last draw record back into the pool
      {
        G->current_draw_record->last_node_with_points->next = G->first_free_node_with_points;
        Node_with_points* node_stack_head_p = G->current_draw_record->first_node_with_points;
        Node_with_points* next_p = node_stack_head_p->next;
        *node_stack_head_p = Node_with_points{};
        node_stack_head_p->next = next_p;
        node_stack_head_p->_is_active = false;
        G->first_free_node_with_points = node_stack_head_p;
      }
      
      Draw_record* record_to_remove = G->current_draw_record;
      G->current_draw_record = G->current_draw_record->prev;

      // Puting the used draw_record back into the pool
      {
        *record_to_remove = Draw_record{};
        record_to_remove->_is_active = false;
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

        // ui_set_next_color({ 107, 75, 10, 255 });
        // UI_PaddedBox(ui_px(5), Axis2__x)
        // {
        //   // Retained state for the edit box
        //   static UI_Text_edit_box_state edit_box_state = {}; 
        //   static U64 buffer_current_size               = 0; 
        //   static U64 cursor_pos                        = 0;
        //   static U64 section_start_pos                 = 0;

        //   Assert(0 < G->pen_size && G->pen_size < 100);
        //   // Scratch scratch = get_scratch(0, 0);
        //   U8 text_buffer[3];
        //   U64 buffer_max_size = ArrayCount(text_buffer);
        //   snprintf((char*)text_buffer, buffer_max_size, "%lld", G->pen_size);

        //   Str8 edit_box_id = Str8FromC("Red color edit box");
        //   UI_Size edit_box_size_x = ui_px(100); 
        //   B32 enter_go_pressed = ui_text_box_mutates(edit_box_id, Str8{}, &edit_box_state, edit_box_size_x, text_buffer, buffer_max_size, &buffer_current_size, &cursor_pos, &section_start_pos, rli_events);
        //   int new_red_color_value = atoi((char*)text_buffer);
        //   if (enter_go_pressed)
        //   {
        //     G->pen_color.r = (F32)new_red_color_value;
        //   }
        // }

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

  G.frame_arena = arena_alloc(Megabytes(64));
  G.pen_size = 11;
  G.font_texture_for_ui = LoadFont("../data/Roboto.ttf");

  G.arena_for_draw_records = arena_alloc(Megabytes(64));
  G.first_draw_record = ArenaCurrentPos(G.arena_for_draw_records, Draw_record);
  
  G.arena_for_nodes_with_points = arena_alloc(Megabytes(64));;
  G.first_node_with_points = ArenaCurrentPos(G.arena_for_nodes_with_points, Node_with_points);

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
    
    // if ui has an active element which we are holding, then we dont start interacting with the app (drawing part)
    
    ui_begin_build((F32)GetScreenWidth(), (F32)GetScreenHeight(), (F32)GetMouseX(), (F32)GetMouseY());
    ui_push_font(G.font_texture_for_ui);

    UI_PaddedBox(ui_px(200), Axis2__x)
    {
      static U8 buffer[100] = {};
      static U64 current_size = 0;
      static U64 cursor = 0;
      static U64 section = 0;

      Scratch scratch = get_scratch(0, 0);
      UI_Text_op_list text_ops = ui_text_op_list_from_rli_events(scratch.arena, rli_events);
      ui_aply_text_ops(text_ops, buffer, ArrayCount(buffer), &current_size, &cursor, &section);
      end_scratch(&scratch);

      UI_Text_edit_box_style edit_box_style = {};
      edit_box_style.font        = ui_get_font();
      edit_box_style.font_size   = ui_get_font_size();
      edit_box_style.width_in_px = 100;

      static U64 left_wall = 0;
      static U64 right_wall = 0;

      ui_push_text_color({ 255, 0, 255, 255 });
      // ui_push_color({ 50, 50, 50, 255 });
      ui_text_edit_box(&edit_box_style, buffer, ArrayCount(buffer), current_size, cursor, section, rli_events, &left_wall, &right_wall); 
    }

    ui_end_build();


    // if (!G.is_mid_drawing)
    // {
    //   update_pencil_ui(&G, rli_events);
    // }

    // update_pencil(&G, ui_has_active());

    DeferLoop(BeginDrawing(), EndDrawing())
    {
      Texture draw_texture = G.draw_texture;
      ClearBackground(BLUE);
      DrawTexturePro(draw_texture, Rectangle{0, 0, (F32)draw_texture.width, (F32)draw_texture.height}, Rectangle{0, 0, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, {}, {}, WHITE);   
      DrawFPS(0, 0);
      ui_draw();
    }
  }


  // note: not releasing stuff, defering it to the system
  return 0;
}