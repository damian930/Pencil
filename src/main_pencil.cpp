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

#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define Rectangle Win32Rectangle
#include "windows.h"
#undef CloseWindow
#undef ShowCursor
#undef Rectangle

LRESULT hook_proc_test(
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
) {
  return {};
}

int main()
{
  os_init();
  allocate_thread_context();
  os_win32_set_thread_context(get_thread_context()); // todo: This is a flaw of the os system and the fact that we link in
  ui_init();
  
  InitWindow(800, 600, "Pencil");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  // SetWindowState(FLAG_WINDOW_UNDECORATED);
  // SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
  // SetWindowState(FLAG_WINDOW_TOPMOST);
  // SetWindowState(FLAG_WINDOW_TRANSPARENT);
  // SetWindowState(FLAG_WINDOW_MAXIMIZED);

  HHOOK hook_handle = SetWindowsHookExA(WH_CALLWNDPROC, hook_proc_test, 0, GetThreadId(GetCurrentThread()));
  Assert(hook_handle != NULL);
  BreakPoint();

  Arena* frame_arena = arena_alloc(Megabytes(64));
  for (;!WindowShouldClose();)
  {
    arena_clear(frame_arena);

    OS_Image os_image = os_take_screenshot(frame_arena);
    Texture screen_shot_texture = {};
    {
      Image rl_image = {};
      rl_image.data    = os_image.data;
      rl_image.format  = PixelFormat_UNCOMPRESSED_R8G8B8A8; 
      rl_image.width   = (int)os_image.width;
      rl_image.height  = (int)os_image.height;
      rl_image.mipmaps = 1;
      screen_shot_texture = LoadTextureFromImage(rl_image);
    }

    if (IsKeyDown(KEY_A)) { printf("Event \n"); }
    if (IsKeyDown(KEY_ENTER)) { 
      make_window_not_capture_events(); 
    }

    DeferLoop(BeginDrawing(), EndDrawing())
    {   
      DrawTexturePro(screen_shot_texture, Rectangle{0, 0, (F32)screen_shot_texture.width, (F32)screen_shot_texture.height}, Rectangle{0.0f, 0.0f, (F32)GetScreenWidth(), (F32)GetScreenHeight()}, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }

    UnloadTexture(screen_shot_texture);
  }

  CloseWindow();

  ui_release();  
  release_thread_context();
  os_release();
  return 0;
}