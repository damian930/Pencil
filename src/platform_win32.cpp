#ifndef PLATFORM_WIN32_CPP
#define PLATFORM_WIN32_CPP 

#include "platform.h"
#pragma comment (lib, "user32.lib")

// #pragma comment (lib, "d3d11.lib")
// #pragma comment (lib, "dxgi.lib")
// #pragma comment (lib, "dxguid.lib")    // This is for the ids for the all the interfaces
// #pragma comment (lib, "d3dcompiler.lib")

struct OS_State {
  WNDCLASSEXA window_class;
  HWND window_handle;
  HINSTANCE app_instance;
  
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
V2S32 os_get_mouse_pos_rel_to_screen()
{
  POINT p = {};
  BOOL succ = GetCursorPos(&p);
  Assert(succ);
  return v2s32(p.x, p.y);
}

V2S32 os_get_mouse_pos_rel_to_client_area()
{
  BOOL succ = false;
  POINT p = {};

  succ = GetCursorPos(&p);
  Assert(succ);

  succ = ScreenToClient(os_get_state()->window_handle, &p);
  Assert(succ);

  V2S32 result = {};
  result.x = p.x;
  result.y = p.y;
  return result;
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

    case WM_CLOSE: 
    {
      DestroyWindow(window_handle);
    } break;

    case WM_DESTROY: 
    {
      PostQuitMessage(0);
    } break;
  }

  OS_Event* ev = ArenaPush(win32_state->frame_events_arena, OS_Event);
  memcpy(ev, &new_ev, sizeof(OS_Event));

  DllPushBack(&win32_state->frame_events, ev);

  return result;
}

// todo: Revise this, this might not clear all the above created stuff on fail


#endif