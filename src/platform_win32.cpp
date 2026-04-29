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

  // Frame data
  // Arena* frame_events_arena;
  // OS_Event_list frame_events;
  //
  V2S32 this_frame_mouse_pos;
  V2S32 prev_frame_mouse_pos;
  //
  OS_Key_state key_states[OS_Key__COUNT];
  OS_Mouse_button_state mouse_button_states[OS_Mouse_button__COUNT];
};

OS_State __os_g_state = {};

void os_init()
{
  // __os_g_state.frame_events_arena = arena_alloc(Megabytes(64));

  // Setting keys
  StaticAssert(ArrayCount(__os_g_state.key_states) == OS_Key__COUNT);
  for (U64 i = 0; i < OS_Key__COUNT; i += 1) 
  {
    OS_Key_state* key_state = __os_g_state.key_states + i;
    key_state->key = (OS_Key)((U32)OS_Key__NONE + i);
    key_state->is_up = true;
  }

  // Setting mouse buttons
  StaticAssert(ArrayCount(__os_g_state.mouse_button_states) == OS_Mouse_button__COUNT);
  for (U64 i = 0; i < OS_Mouse_button__COUNT; i += 1) 
  {
    OS_Mouse_button_state* button_state = __os_g_state.mouse_button_states + i;
    button_state->button = (OS_Mouse_button)((U32)OS_Mouse_button__NONE + i);
    button_state->is_up = true;
  }
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

// AGI here
Str8 str8_from_os_key(OS_Key key)
{ 
  Str8 str = {};
  switch (key)
  {
    default: { str = Str8FromC("__UNMATCHED__"); Assert(0); } break;

    case OS_Key__NONE:          { str = Str8FromC("__NONE__");        } break;
    case OS_Key__Shift:         { str = Str8FromC("__SHIFT__");       } break;
    case OS_Key__Control:       { str = Str8FromC("__CONTROL__");     } break;

    // Letters a-z
    case OS_Key__A: { str = Str8FromC("a"); } break;
    case OS_Key__B: { str = Str8FromC("b"); } break;
    case OS_Key__C: { str = Str8FromC("c"); } break;
    case OS_Key__D: { str = Str8FromC("d"); } break;
    case OS_Key__E: { str = Str8FromC("e"); } break;
    case OS_Key__F: { str = Str8FromC("f"); } break;
    case OS_Key__G: { str = Str8FromC("g"); } break;
    case OS_Key__H: { str = Str8FromC("h"); } break;
    case OS_Key__I: { str = Str8FromC("i"); } break;
    case OS_Key__J: { str = Str8FromC("j"); } break;
    case OS_Key__K: { str = Str8FromC("k"); } break;
    case OS_Key__L: { str = Str8FromC("l"); } break;
    case OS_Key__M: { str = Str8FromC("m"); } break;
    case OS_Key__N: { str = Str8FromC("n"); } break;
    case OS_Key__O: { str = Str8FromC("o"); } break;
    case OS_Key__P: { str = Str8FromC("p"); } break;
    case OS_Key__Q: { str = Str8FromC("q"); } break;
    case OS_Key__R: { str = Str8FromC("r"); } break;
    case OS_Key__S: { str = Str8FromC("s"); } break;
    case OS_Key__T: { str = Str8FromC("t"); } break;
    case OS_Key__U: { str = Str8FromC("u"); } break;
    case OS_Key__V: { str = Str8FromC("v"); } break;
    case OS_Key__W: { str = Str8FromC("w"); } break;
    case OS_Key__X: { str = Str8FromC("x"); } break;
    case OS_Key__Y: { str = Str8FromC("y"); } break;
    case OS_Key__Z: { str = Str8FromC("z"); } break;

    // Numbers 0-9
    case OS_Key__0: { str = Str8FromC("0"); } break;
    case OS_Key__1: { str = Str8FromC("1"); } break;
    case OS_Key__2: { str = Str8FromC("2"); } break;
    case OS_Key__3: { str = Str8FromC("3"); } break;
    case OS_Key__4: { str = Str8FromC("4"); } break;
    case OS_Key__5: { str = Str8FromC("5"); } break;
    case OS_Key__6: { str = Str8FromC("6"); } break;
    case OS_Key__7: { str = Str8FromC("7"); } break;
    case OS_Key__8: { str = Str8FromC("8"); } break;
    case OS_Key__9: { str = Str8FromC("9"); } break;

    // Other printable
    case OS_Key__Space:         { str = Str8FromC(" ");  } break;
    case OS_Key__Backtick:      { str = Str8FromC("`");  } break;
    case OS_Key__Minus:         { str = Str8FromC("-");  } break;
    case OS_Key__Equals:        { str = Str8FromC("=");  } break;
    case OS_Key__Left_bracket:  { str = Str8FromC("[");  } break;
    case OS_Key__Right_bracket: { str = Str8FromC("]");  } break;
    case OS_Key__Backslash:     { str = Str8FromC("\\"); } break;
    case OS_Key__Semicolon:     { str = Str8FromC(";");  } break;
    case OS_Key__Apostrophe:    { str = Str8FromC("'");  } break;
    case OS_Key__Comma:         { str = Str8FromC(",");  } break;
    case OS_Key__Period:        { str = Str8FromC(".");  } break;
    case OS_Key__Slash:         { str = Str8FromC("/");  } break;

    // Non-printable
    case OS_Key__Left_arrow:    { str = Str8FromC("__LEFT_ARROW__");  } break;
    case OS_Key__Right_arrow:   { str = Str8FromC("__RIGHT_ARROW__"); } break;
    case OS_Key__Up_arrow:      { str = Str8FromC("__UP_ARROW__");    } break;
    case OS_Key__Down_arrow:    { str = Str8FromC("__DOWN_ARROW__");  } break;
    case OS_Key__Home:          { str = Str8FromC("__HOME__");        } break;
    case OS_Key__End:           { str = Str8FromC("__END__");         } break;
    case OS_Key__Page_up:       { str = Str8FromC("__PAGE_UP__");     } break;
    case OS_Key__Page_down:     { str = Str8FromC("__PAGE_DOWN__");   } break;
    case OS_Key__Backspace:     { str = Str8FromC("__BACKSPACE__");   } break;
    case OS_Key__Delete:        { str = Str8FromC("__DELETE__");      } break;
    case OS_Key__Insert:        { str = Str8FromC("__INSERT__");      } break;
    case OS_Key__Escape:        { str = Str8FromC("__ESCAPE__");      } break;
    case OS_Key__Tab:           { str = Str8FromC("__TAB__");         } break;
    case OS_Key__Enter:         { str = Str8FromC("__ENTER__");       } break;
    case OS_Key__Caps_lock:     { str = Str8FromC("__CAPS_LOCK__");   } break;
  }
  return str;
}

LRESULT win_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) { 
  OS_State* win32_state = os_get_state();

  // OS_Event new_ev = {};

  LRESULT result = {};
  switch (message)
  {
    default: { result = DefWindowProc(window_handle, message, w_param, l_param); } break;
    
    case WM_KEYDOWN: 
    case WM_KEYUP: 
    {
      // win32_state->key_states + 
      B32 key_found = true;
      OS_Key key = {};

      if ('A' <= w_param && w_param <= 'Z')
      {
        key = (OS_Key)((U32)OS_Key__A + (w_param - 'A'));
      }
      else { key_found = false; }

      // Unwrapping the message data
      B32 went_down = false;
      B32 went_up = false;
      {
        U16 key_repeat_count = (U16)l_param;
        U8 scan_code         = (U8)(l_param >> 16);
        B32 is_extended_key  = l_param & bit_24;
        B32 context_code     = l_param & bit_29;
        B32 prev_key_state   = l_param & bit_30;
        B32 transition_state = l_param & bit_31; // Always 0 for WM_KEYDOWN

        went_down = (transition_state == 0);
        went_up = (transition_state != 0);
        XOR(went_down, went_up); // Just making sure
      }

      if (key_found)
      {
        OS_Key_state* key_state = win32_state->key_states + key;
        key_state->was_down = key_state->is_down;
        key_state->was_up = key_state->is_up;

        if (went_down) { 
          key_state->is_down = true;
          key_state->is_up = false;
        }
        else if (went_up) {
          key_state->is_down = false;
          key_state->is_up = true;
        }
      }

    } break;

    /*
    case WM_LBUTTONDOWN:
    {
      // new_ev.kind = OS_Event_kind__Mouse_went_down;

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
    */

    /*
    case WM_LBUTTONUP:
    {
      // new_ev.kind = OS_Event_kind__Mouse_went_up;

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
    */

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

  // OS_Event* ev = ArenaPush(win32_state->frame_events_arena, OS_Event);
  // memcpy(ev, &new_ev, sizeof(OS_Event));

  // DllPushBack(&win32_state->frame_events, ev);

  return result;
}

OS_Key_state os_get_key_state(OS_Key key)
{
  OS_State* os = os_get_state();
  OS_Key_state key_state = os->key_states[key];
  return key_state;
}

B32 os_window_should_close()
{
  return os_get_state()->os_should_close_window;
}

void os_win32_frame_begin()
{
  OS_State* os_state = os_get_state();
  
  // Creating frame events though the winproc
  for (MSG msg = {};PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  // // Mouse positions
  // os_state->prev_frame_mouse_pos = os_state->this_frame_mouse_pos;
  // {
  //   // todo:
  //   // os_state->this_frame_mouse_pos = v2s32(f32_is_nan);
  //   POINT p = {};
  //   BOOL succ = {};
  //   succ = GetCursorPos(&p); Assert(succ);
  //   succ = ScreenToClient(os_get_state()->window_handle, &p); Assert(succ);
  //   // note: When window is closed we cant get relative to widnow,
  //   //       do i want to create a value for the invalid mosue pos that will have some like nan for the float
  //   os_state->this_frame_mouse_pos = v2s32(p.x, p.y);
  // }

  // // Key states
  // StaticAssert(ArrayCount(os_state->key_states) == OS_Key__COUNT);
  // // for (os_state->frame_events;)
  // // for (U64 i = 0; i < OS_Key__COUNT; i += 1) 
  // // {


  // // }

}


#endif