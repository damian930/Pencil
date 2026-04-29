#ifndef PLATFORM_WIN32_CPP
#define PLATFORM_WIN32_CPP 

#include "platform.h"
#pragma comment (lib, "user32.lib")

// #pragma comment (lib, "d3d11.lib")
// #pragma comment (lib, "dxgi.lib")
// #pragma comment (lib, "dxguid.lib")    // This is for the ids for the all the interfaces
// #pragma comment (lib, "d3dcompiler.lib")

struct OS_State {
  HINSTANCE app_instance;
  
  // Single window data
  WNDCLASSEXA window_class;
  HWND window_handle;
  B32 os_should_close_window;

  V2S32 this_frame_mouse_pos;
  V2S32 prev_frame_mouse_pos;

  // OS_Mouse_button_states mouse_button_states;
  Arena* frame_events_arena;
  OS_Event_list frame_events;
};

OS_State __os_g_state = {};

void os_init()
{
  __os_g_state.frame_events_arena = arena_alloc(Megabytes(64));
}

OS_State* os_get_state()
{
  return &__os_g_state;
}

///////////////////////////////////////////////////////////
// - Misc
//
V2S32 os_get_client_area_dims()
{
  RECT rect = {};
  BOOL succ = GetClientRect(__os_g_state.window_handle, &rect);
  Assert(succ);
  V2S32 result = v2s32(rect.right - rect.left, rect.bottom - rect.top);
  return result;
}

// note: This gets them in the screen coordinates
/*
V2S32 os_get_mouse_pos_rel_to_screen()
{
  POINT p = {};
  BOOL succ = GetCursorPos(&p);
  Assert(succ);
  return v2s32(p.x, p.y);
}
*/

// note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_mouse_pos()
{
  V2S32 s32_pos = os_get_state()->this_frame_mouse_pos;
  V2F32 f32_pos = v2f32((F32)s32_pos.x, (F32)s32_pos.y);
  return f32_pos;
}

// note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_prev_mouse_pos()
{
  V2S32 s32_pos = os_get_state()->prev_frame_mouse_pos;
  V2F32 f32_pos = v2f32((F32)s32_pos.x, (F32)s32_pos.y);
  return f32_pos;
}

V2F32 os_get_mouse_delta()
{
  V2F32 this_frame = os_get_mouse_pos();
  V2F32 prev_frame = os_get_prev_mouse_pos();
  V2F32 delta = {};
  delta.x = this_frame.x - prev_frame.x;
  delta.y = this_frame.y - prev_frame.y;
  return delta;
}

LRESULT win_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) { 
  OS_State* win32_state = os_get_state();

  OS_Event new_ev = {};

  LRESULT result = {};
  switch (message)
  {
    default: { result = DefWindowProc(window_handle, message, w_param, l_param); } break;
    
    case WM_LBUTTONDOWN:
    {
      new_ev.kind = OS_Event_kind__Mouse_went_down;

      B32 is_control_down           = w_param & MK_CONTROL;
      B32 is_shift_down             = w_param & MK_SHIFT;
      B32 is_mouse_left_down        = w_param & MK_LBUTTON;
      B32 is_mouse_middle_down      = w_param & MK_MBUTTON;
      B32 is_mouse_right_down       = w_param & MK_RBUTTON;
      B32 is_side_button_back_down  = w_param & MK_XBUTTON1;
      B32 is_side_button_front_down = w_param & MK_XBUTTON1;
      Assert(is_mouse_left_down); // Making sure that this is always true

      if (is_control_down) { new_ev.mods |= OS_Event_modifier__Control; }
      if (is_shift_down) { new_ev.mods |= OS_Event_modifier__Shift; }

      // note: These are signed cause x/y might be negative for multiple monitors
      int x = GET_X_LPARAM(l_param);
      int y = GET_Y_LPARAM(l_param);
      new_ev.mouse_pos = v2s32(x, y);
    } break;

    case WM_LBUTTONUP:
    {
      new_ev.kind = OS_Event_kind__Mouse_went_up;

      B32 is_control_down           = w_param & MK_CONTROL;
      B32 is_shift_down             = w_param & MK_SHIFT;
      B32 is_mouse_middle_down      = w_param & MK_MBUTTON;
      B32 is_mouse_right_down       = w_param & MK_RBUTTON;
      B32 is_side_button_back_down  = w_param & MK_XBUTTON1;
      B32 is_side_button_front_down = w_param & MK_XBUTTON1;

      if (is_control_down) { new_ev.mods |= OS_Event_modifier__Control; }
      if (is_shift_down) { new_ev.mods |= OS_Event_modifier__Shift; }

      // note: These are signed cause x/y might be negative for multiple monitors
      int x = GET_X_LPARAM(l_param);
      int y = GET_Y_LPARAM(l_param);
      new_ev.mouse_pos = v2s32(x, y);
    } break;

    case WM_SIZE: 
    {

    } break;

    case WM_CLOSE: // This is when the close button for the windows default window is pressed 
    {
      win32_state->os_should_close_window = true;
    } break;

    case WM_DESTROY: 
    {
      // todo: This might want to be a separate event or something for the window closing
    } break;
  }

  OS_Event* ev = ArenaPush(win32_state->frame_events_arena, OS_Event);
  memcpy(ev, &new_ev, sizeof(OS_Event));

  DllPushBack(&win32_state->frame_events, ev);

  return result;
}

B32 os_shoud_close_window()
{
  return os_get_state()->os_should_close_window;
}

void os_win32_frame_begin()
{
  OS_State* os_state = os_get_state();
  
  B32 stop_the_app = false;
  (void)stop_the_app;
  MSG msg;
  for (;PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
  {
    if (msg.message == WM_QUIT) { stop_the_app = true; break; }
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  // Mouse positions
  os_state->prev_frame_mouse_pos = os_state->this_frame_mouse_pos;
  {
    // todo:
    // os_state->this_frame_mouse_pos = v2s32(f32_is_nan);
    POINT p = {};
    BOOL succ = {};
    succ = GetCursorPos(&p); Assert(succ);
    succ = ScreenToClient(os_get_state()->window_handle, &p); Assert(succ);
    // note: When window is closed we cant get relative to widnow,
    //       do i want to create a value for the invalid mosue pos that will have some like nan for the float
    os_state->this_frame_mouse_pos = v2s32(p.x, p.y);
  }
}


#endif