#ifndef OS_WIN32_CPP
#define OS_WIN32_CPP 

// todos: Check for 0 handles inside functions

#include "os/win32.h"
#pragma comment (lib, "user32.lib")

// #pragma comment (lib, "d3d11.lib")
// #pragma comment (lib, "dxgi.lib")
// #pragma comment (lib, "dxguid.lib")    // This is for the ids for the all the interfaces
// #pragma comment (lib, "d3dcompiler.lib")

struct OS_State {
  HINSTANCE app_instance;
  U64 perf_freq_count_per_sec;

  // Single window data
  OS_Window window;

  // Frame data
  // Arena* frame_events_arena;
  // OS_Event_list frame_events;
  //
  V2S32 this_frame_mouse_pos;
  V2S32 prev_frame_mouse_pos;
  //
  OS_Key_state key_states[Key__COUNT];
  OS_Mouse_button_state mouse_button_states[Mouse_button__COUNT];
};

OS_State __os_g_state = {};

///////////////////////////////////////////////////////////
// - State
//
void os_init()
{
  // Performance frequency
  LARGE_INTEGER freq_lr = {};
  BOOL succ = QueryPerformanceFrequency(&freq_lr); Assert(succ);
  __os_g_state.perf_freq_count_per_sec = freq_lr.QuadPart;

  // Setting keys
  StaticAssert(ArrayCount(__os_g_state.key_states) == Key__COUNT);
  for (U64 i = 0; i < Key__COUNT; i += 1) 
  {
    OS_Key_state* key_state = __os_g_state.key_states + i;
    key_state->key = (Key)((U32)Key__NONE + i);
    key_state->is_up = true;
  }

  // Setting mouse buttons
  StaticAssert(ArrayCount(__os_g_state.mouse_button_states) == Mouse_button__COUNT);
  for (U64 i = 0; i < Mouse_button__COUNT; i += 1) 
  {
    OS_Mouse_button_state* button_state = __os_g_state.mouse_button_states + i;
    button_state->button = (Mouse_button)((U32)Mouse_button__NONE + i);
    button_state->is_up = true;
  }
}

void os_release()
{
  InvalidCodePath();
}

OS_State* os_get_state()
{
  return &__os_g_state;
}

///////////////////////////////////////////////////////////
// - Frame
//
void os_frame_begin()
{
  OS_State* os_state = os_get_state();
  
  // Setting the prev frame to the old input data 
  {
    StaticAssert(ArrayCount(__os_g_state.key_states) == Key__COUNT);
    for (U64 i = 0; i < Key__COUNT; i += 1) 
    {
      OS_Key_state* key_state = __os_g_state.key_states + i;
      key_state->was_down = key_state->is_down;
      key_state->was_up = key_state->is_up;
    }

    // Setting mouse buttons
    StaticAssert(ArrayCount(__os_g_state.mouse_button_states) == Mouse_button__COUNT);
    for (U64 i = 0; i < Mouse_button__COUNT; i += 1) 
    {
      OS_Mouse_button_state* button_state = __os_g_state.mouse_button_states + i;
      button_state->was_down = button_state->is_down;
      button_state->was_up = button_state->is_up;
    }
  }

  // Creating frame events though the winproc
  for (MSG msg = {};PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
  {
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
    succ = ScreenToClient(os_get_state()->window.handle, &p); Assert(succ);
    // note: When window is closed we cant get relative to widnow,
    //       do i want to create a value for the invalid mosue pos that will have some like nan for the float
    os_state->this_frame_mouse_pos = v2s32(p.x, p.y);
  }

  // // Key states
  // StaticAssert(ArrayCount(os_state->key_states) == OS_Key__COUNT);
  // // for (os_state->frame_events;)
  // // for (U64 i = 0; i < OS_Key__COUNT; i += 1) 
  // // {


  // // }

}

void os_frame_end() {}

///////////////////////////////////////////////////////////
// - Windowing
//
V2F32 os_get_window_dims()
{
  RECT rect = {};
  BOOL succ = GetWindowRect(__os_g_state.window.handle, &rect); Assert(succ);
  V2F32 result = v2f32((F32)(rect.right - rect.left), (F32)(rect.bottom - rect.top));
  return result;
}

V2F32 os_get_client_area_dims()
{
  // todo: Maybe this shoud be stored inside the fram thing to always return the same thing.
  //       Look into this some time
  // note: GetClientRect return RECT that has its right and bottom exclusive. 
  //       Maning that those coordinates lie immediately after the last valid
  //       client area position.
  RECT rect = {};
  BOOL succ = GetClientRect(__os_g_state.window.handle, &rect); Assert(succ);
  V2F32 result = v2f32((F32)(rect.right - rect.left), (F32)(rect.bottom - rect.top));
  return result;
}

V2F32 os_get_mouse_pos()
{
  V2S32 s32_pos = os_get_state()->this_frame_mouse_pos;
  V2F32 f32_pos = v2f32((F32)s32_pos.x, (F32)s32_pos.y);
  return f32_pos;
}

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

B32 os_window_should_close()
{
  return os_get_state()->window.shoud_close;
}

void os_window_maximize()
{
  BOOL succ = ShowWindow(os_get_state()->window.handle, SW_MAXIMIZE);
  Assert(succ);
}

void os_window_minimize()
{
  BOOL succ = ShowWindow(os_get_state()->window.handle, SW_MINIMIZE);
  Assert(succ);
}

void os_window_set_mouse_passthrough(B32 enable)
{
  // This is taked from glfw, here is the link to how they do it:
  // https://github.com/glfw/glfw/blob/b00e6a8a88ad1b60c0a045e696301deb92c9a13e/src/win32_window.c#L2007
  // That function implements the win32 specific function that allows a window to pass through inputs.
  // ---
  // Here is documentation to win32 docs that specify what style of window shoud be used for the 
  // mouse event passing through to take place:
  // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#layered-windows:~:text=Hit%20testing%20of,the%20layered%20window.

  OS_State* os_state = os_get_state();

  LONG_PTR styles = GetWindowLongPtr(os_state->window.handle, GWL_EXSTYLE);

  if ((styles & WS_EX_LAYERED) && !(styles & WS_EX_TRANSPARENT)) { 
    InvalidCodePath();
    // Not yet implemented this case. 
    // For now built in abstraction only allows for LAYERS to be applied here and removed here. 
    // Its invalid here, since if the window is layers and might have some alpha aplied to it, 
    // this function is not made to reset the alpha when removing TRANSPARENT.
    // I could handle it here, but i would have to test stuff to see if it works,
    // instead i just break here to see if i ever need this type of stuff.  
  }

  if (enable) {
    styles |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
  }
  else {
    styles &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);
  }

  SetWindowLongPtrW(os_state->window.handle, GWL_EXSTYLE, styles);
}

B32 os_window_is_mouse_passthrough()
{
  OS_State* os_state = os_get_state();
  LONG_PTR styles = GetWindowLongPtr(os_state->window.handle, GWL_EXSTYLE);
  B32 result = !!(styles & (WS_EX_TRANSPARENT | WS_EX_LAYERED));
  return result;
}

// void os_window_set_top_most(B32 b)
// {
//   const HWND after = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
//     SetWindowPos(window->win32.handle, after, 0, 0, 0, 0,
//                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
// }

///////////////////////////////////////////////////////////
// - Inputs for keyboard
//
OS_Key_state os_get_key_state(Key key)
{
  OS_State* os = os_get_state();
  OS_Key_state key_state = os->key_states[key];
  return key_state;
}

B32 os_key_down(Key key)
{
  OS_Key_state state = os_get_key_state(key);
  return state.is_down;
}

B32 os_key_up(Key key)
{
  OS_Key_state state = os_get_key_state(key);
  return state.is_up;
}

B32 os_key_went_down(Key key)
{
  OS_Key_state state = os_get_key_state(key);
  return (state.was_up && state.is_down );
}

B32 os_key_went_up(Key key)
{
  OS_Key_state state = os_get_key_state(key);
  return (state.was_down && state.is_up);
}

Str8 str8_from_os_key(Key key)
{ 
  Str8 str = {};
  switch (key)
  {
    default: { str = Str8FromC("__UNMATCHED__"); Assert(0); } break;

    case Key__NONE:    { str = Str8FromC("__NONE__");    } break;
    case Key__Shift:   { str = Str8FromC("__SHIFT__");   } break;
    case Key__Control: { str = Str8FromC("__CONTROL__"); } break;

    // Letters a-z
    case Key__A: { str = Str8FromC("a"); } break;
    case Key__B: { str = Str8FromC("b"); } break;
    case Key__C: { str = Str8FromC("c"); } break;
    case Key__D: { str = Str8FromC("d"); } break;
    case Key__E: { str = Str8FromC("e"); } break;
    case Key__F: { str = Str8FromC("f"); } break;
    case Key__G: { str = Str8FromC("P"); } break;
    case Key__H: { str = Str8FromC("h"); } break;
    case Key__I: { str = Str8FromC("i"); } break;
    case Key__J: { str = Str8FromC("j"); } break;
    case Key__K: { str = Str8FromC("k"); } break;
    case Key__L: { str = Str8FromC("l"); } break;
    case Key__M: { str = Str8FromC("m"); } break;
    case Key__N: { str = Str8FromC("n"); } break;
    case Key__O: { str = Str8FromC("o"); } break;
    case Key__P: { str = Str8FromC("p"); } break;
    case Key__Q: { str = Str8FromC("q"); } break;
    case Key__R: { str = Str8FromC("r"); } break;
    case Key__S: { str = Str8FromC("s"); } break;
    case Key__T: { str = Str8FromC("t"); } break;
    case Key__U: { str = Str8FromC("u"); } break;
    case Key__V: { str = Str8FromC("v"); } break;
    case Key__W: { str = Str8FromC("w"); } break;
    case Key__X: { str = Str8FromC("x"); } break;
    case Key__Y: { str = Str8FromC("y"); } break;
    case Key__Z: { str = Str8FromC("z"); } break;

    // Numbers 0-9
    case Key__0: { str = Str8FromC("0"); } break;
    case Key__1: { str = Str8FromC("1"); } break;
    case Key__2: { str = Str8FromC("2"); } break;
    case Key__3: { str = Str8FromC("3"); } break;
    case Key__4: { str = Str8FromC("4"); } break;
    case Key__5: { str = Str8FromC("5"); } break;
    case Key__6: { str = Str8FromC("6"); } break;
    case Key__7: { str = Str8FromC("7"); } break;
    case Key__8: { str = Str8FromC("8"); } break;
    case Key__9: { str = Str8FromC("9"); } break;

    // Other printable
    case Key__Space:         { str = Str8FromC(" ");  } break;
    case Key__Backtick:      { str = Str8FromC("`");  } break;
    case Key__Minus:         { str = Str8FromC("-");  } break;
    case Key__Equals:        { str = Str8FromC("=");  } break;
    case Key__Left_bracket:  { str = Str8FromC("[");  } break;
    case Key__Right_bracket: { str = Str8FromC("]");  } break;
    case Key__Backslash:     { str = Str8FromC("\\"); } break;
    case Key__Semicolon:     { str = Str8FromC(";");  } break;
    case Key__Apostrophe:    { str = Str8FromC("'");  } break;
    case Key__Comma:         { str = Str8FromC(",");  } break;
    case Key__Period:        { str = Str8FromC(".");  } break;
    case Key__Slash:         { str = Str8FromC("/");  } break;

    // Non-printable
    case Key__Left_arrow:    { str = Str8FromC("__LEFT_ARROW__");  } break;
    case Key__Right_arrow:   { str = Str8FromC("__RIGHT_ARROW__"); } break;
    case Key__Up_arrow:      { str = Str8FromC("__UP_ARROW__");    } break;
    case Key__Down_arrow:    { str = Str8FromC("__DOWN_ARROW__");  } break;
    case Key__Home:          { str = Str8FromC("__HOME__");        } break;
    case Key__End:           { str = Str8FromC("__END__");         } break;
    case Key__Page_up:       { str = Str8FromC("__PAGE_UP__");     } break;
    case Key__Page_down:     { str = Str8FromC("__PAGE_DOWN__");   } break;
    case Key__Backspace:     { str = Str8FromC("__BACKSPACE__");   } break;
    case Key__Delete:        { str = Str8FromC("__DELETE__");      } break;
    case Key__Insert:        { str = Str8FromC("__INSERT__");      } break;
    case Key__Escape:        { str = Str8FromC("__ESCAPE__");      } break;
    case Key__Tab:           { str = Str8FromC("__TAB__");         } break;
    case Key__Enter:         { str = Str8FromC("__ENTER__");       } break;
    case Key__Caps_lock:     { str = Str8FromC("__CAPS_LOCK__");   } break;
  }
  return str;
}

///////////////////////////////////////////////////////////
// - Inputs for mouse 
//
OS_Mouse_button_state os_get_mouse_button_state(Mouse_button button)
{
  OS_State* os = os_get_state();
  OS_Mouse_button_state button_state = os->mouse_button_states[button];
  return button_state;
}

B32 os_mouse_button_down(Mouse_button button)
{
  OS_Mouse_button_state state = os_get_mouse_button_state(button);
  return state.is_down;
}

B32 os_mouse_button_up(Mouse_button button)
{
  OS_Mouse_button_state state = os_get_mouse_button_state(button);
  return state.is_up;
}

B32 os_mouse_button_went_down(Mouse_button button)
{
  OS_Mouse_button_state state = os_get_mouse_button_state(button);
  return (state.is_down && state.was_up);
}

B32 os_mouse_button_went_up(Mouse_button button)
{
  OS_Mouse_button_state state = os_get_mouse_button_state(button);
  return (state.is_up && state.was_down);
}

///////////////////////////////////////////////////////////
// - Time
//
Readable_time os_get_readable_time()
{
  SYSTEMTIME sys_time = {};
  GetLocalTime(&sys_time);

  Readable_time time = {};
  time.year       = sys_time.wYear;
  time.month      = (Month)(sys_time.wMonth - 1);
  time.day        = (U8)sys_time.wDay;
  time.hour       = (U8)sys_time.wHour;
  time.minute     = (U8)sys_time.wMinute;
  time.second     = (U8)sys_time.wSecond;
  time.milisecond = sys_time.wMilliseconds;
  
  return time;
}

Time os_get_time_ms()
{
  Readable_time r_time = os_get_readable_time();
  Time time = time_from_readable_time(&r_time); 
  return time;
}

U64 os_get_perf_counter()
{
  LARGE_INTEGER lr_end = {};
  BOOL succ = QueryPerformanceCounter(&lr_end); Assert(succ);
  return lr_end.QuadPart;
}

U64 os_get_perf_freq_per_sec()
{
  OS_State* state = os_get_state();
  return state->perf_freq_count_per_sec;
}

void os_sleep(U64 ms)
{
  Assert(ms <= u32_max);
  ms = Min(ms, u32_max);
  Sleep((U32)ms);
}

///////////////////////////////////////////////////////////
// - Misc
//
LRESULT win32_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) { 
  OS_State* win32_state = os_get_state();

  LRESULT result = {};
  switch (message)
  {
    default: { result = DefWindowProc(window_handle, message, w_param, l_param); } break;
    
    case WM_KEYDOWN: 
    case WM_KEYUP: 
    {
      Key key = {};

      if ('A' <= w_param && w_param <= 'Z') { key = (Key)((U32)Key__A + (w_param - 'A')); }
      else {
        switch (w_param)
        {
          default:         { /*InvalidCodePath();*/  } break;
          case VK_SHIFT:   { key = Key__Shift;   } break;
          case VK_CONTROL: { key = Key__Control; } break;
          case VK_DELETE:  { key = Key__Delete; } break;
        }
      }
      // Assert(key != Key__NONE);

      // Unwrapping the message data
      B32 went_down = false;
      B32 went_up   = false;
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

      if (key != Key__NONE)
      {
        // note: was down/up are not touched here, since they are updated in the frame_begin routine
        
        OS_Key_state* key_state = win32_state->key_states + key;
        Assert(key_state->key == key);

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

    case WM_LBUTTONDOWN: case WM_LBUTTONUP: 
    case WM_RBUTTONDOWN: case WM_RBUTTONUP:
    case WM_MBUTTONDOWN: case WM_MBUTTONUP:
    case WM_XBUTTONDOWN: case WM_XBUTTONUP:
    {
      Mouse_button button = Mouse_button__NONE;
      B8 went_down = false;
      B8 went_up = false;

      if (message == WM_LBUTTONDOWN) { button = Mouse_button__Left;   went_down = true; }
      if (message == WM_RBUTTONDOWN) { button = Mouse_button__Right;  went_down = true; }
      if (message == WM_MBUTTONDOWN) { button = Mouse_button__Middle; went_down = true; }
      if (message == WM_XBUTTONDOWN) { InvalidCodePath(); }

      if (message == WM_LBUTTONUP) { button = Mouse_button__Left;   went_up = true; }
      if (message == WM_RBUTTONUP) { button = Mouse_button__Right;  went_up = true; }
      if (message == WM_MBUTTONUP) { button = Mouse_button__Middle; went_up = true; }
      if (message == WM_XBUTTONUP) { InvalidCodePath(); }

      // was down/up are not touched here, since they are updated in the frame_begin routine

      OS_Mouse_button_state* button_state = win32_state->mouse_button_states + button;
      button_state->is_down = went_down;
      button_state->is_up   = went_up;
    } break;

    case WM_NCCALCSIZE:
    {
      result = DefWindowProc(window_handle, message, w_param, l_param);
      // todo: Get the caption size from the system and only remove the caption size on y.
      //       This is needed to have the resize buttons and all that be present.
    } break;

    case WM_ACTIVATEAPP: // note: Message that out window is about to be activated or is not longer active
    {
      result = DefWindowProc(window_handle, message, w_param, l_param);
    } break;

    case WM_SIZE: 
    {

    } break;

    case WM_CLOSE: // For regular windows this is send when the close button it pressed 
    {
      win32_state->window.shoud_close = true;
    } break;

    case WM_DESTROY: 
    {
      // todo: This might want to be a separate event or something for the window closing
      // todo: This shoud be called inside something like close_window
    } break;
  }

  // OS_Event* ev = ArenaPush(win32_state->frame_events_arena, OS_Event);
  // memcpy(ev, &new_ev, sizeof(OS_Event));

  // DllPushBack(&win32_state->frame_events, ev);

  return result;
}


///////////////////////////////////////////////////////////
// - D3D 
//
D3D_Program d3d_program_from_file(
  ID3D11Device* d3d_device,
  const WCHAR* shader_program_file, 
  const char* v_shader_main_f_name,
  const char* p_shader_main_f_name,
  B32* out_opt_is_succ
) {
  B32 is_succ = true;

  // Getting v shader compiled
  ID3DBlob* v_blob = 0;
  if (is_succ)
  {
    ID3DBlob* error_blob = 0;

    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "vs_main", "vs_5_0", Null, Null, &v_blob, &error_blob
    );

    if (error_blob != 0 || hr != S_OK)
    {
      if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
      is_succ = false;
    }

    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3DBlob* p_blob = 0;
  if (is_succ)
  {
    ID3DBlob* error_blob = 0;
  
    HRESULT hr = D3DCompileFromFile(
      shader_program_file, Null, Null,
      "ps_main", "ps_5_0", Null, Null, &p_blob, &error_blob
    );

    if (error_blob != 0 || hr != S_OK)
    {
      if (error_blob != 0) { OutputDebugStringA((char*)error_blob->GetBufferPointer()); }
      is_succ = false;
    }

    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3D11VertexShader* v_shader = 0;
  ID3D11PixelShader* p_shader = 0;
  if (is_succ)
  {
    HRESULT hr = S_OK;
    hr = d3d_device->CreateVertexShader(v_blob->GetBufferPointer(), v_blob->GetBufferSize(), Null, &v_shader);
    if (hr != S_OK) { is_succ = false; }
    hr = d3d_device->CreatePixelShader(p_blob->GetBufferPointer(), p_blob->GetBufferSize(), Null, &p_shader);
    if (hr != S_OK) { is_succ = false; }
    
    // D3D11_INPUT_ELEMENT_DESC layout_decs = {};
    // hr = d3d_device->CreateInputLayout(&layout_decs, 0, v_blob->GetBufferPointer(), v_blob->GetBufferSize(), &grafics.input_layout_for_rect_program);
    // HR(hr);
  }

  if (p_blob != 0) { p_blob->Release(); }  
  if (v_blob != 0) { v_blob->Release(); }  

  // // Creating a uniform buffer
  // D3D11_BUFFER_DESC uniform_desc = {};
  // uniform_desc.ByteWidth      = sizeof(D3D_Rect_program_data);
  // uniform_desc.Usage          = D3D11_USAGE_DYNAMIC;
  // uniform_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  // uniform_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  // d3d_device->CreateBuffer(&uniform_desc, NULL, &grafics.uniform_buffer_for_rect_program);

  if (out_opt_is_succ != 0) { *out_opt_is_succ = is_succ; }
  D3D_Program program = {};
  program.v_shader     = v_shader;
  program.p_shader     = p_shader;
  return program;
}

ID3D11RenderTargetView* d3d_get_frame_buffer_rtv(D3D_State* d3d)
{
  ID3D11RenderTargetView* frame_buffer_rtv = 0;
  ID3D11Texture2D* backbuffer;
  d3d->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
  d3d->device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &frame_buffer_rtv);
  backbuffer->Release();
  return frame_buffer_rtv;
}

ID3D11RenderTargetView* d3d_make_rtv(D3D_State* d3d, U32 width, U32 height)
{
  ID3D11Texture2D* texture = 0;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = d3d->device->CreateTexture2D(&desc, Null, &texture);
    Assert(hr == S_OK);
  }

  ID3D11RenderTargetView* rtv = 0;
  {
    HRESULT hr = d3d->device->CreateRenderTargetView((ID3D11Resource*)texture, 0, &rtv);
    Assert(hr == S_OK);
  }

  texture->Release();
  return rtv;
}

D3D_Texture_result d3d_texture_from_rtv(ID3D11RenderTargetView* rtv)
{
  HRESULT hr = S_OK;
  
  ID3D11Resource* resource = 0;
  rtv->GetResource(&resource);

  ID3D11Texture2D* texture = 0;
  hr = resource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);

  D3D_Texture_result result = {};
  result.texture = texture;
  result.succ    = (hr == S_OK);

  resource->Release();
  return result;
}

Image_buffer d3d_export_texture(Arena* arena, D3D_State* d3d, ID3D11Texture2D* src_texture)
{
  HRESULT hr = S_OK;

  Scratch scratch = get_scratch(0, 0);
  ID3D11Texture2D* copy_texture = 0;

  U64 texture_height = 0;
  U64 texture_width  = 0;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    src_texture->GetDesc(&desc);
    desc.BindFlags      = 0;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    texture_height = (U64)desc.Height;
    texture_width  = (U64)desc.Width;
    format         = desc.Format;

    hr = d3d->device->CreateTexture2D(&desc, 0, &copy_texture);
    HandleLater(hr == S_OK);

    d3d->context->CopyResource((ID3D11Resource*)copy_texture, (ID3D11Resource*)src_texture);
  }

  // note: For now only this one
  HandleLater(format == DXGI_FORMAT_R8G8B8A8_UNORM);
  U64 bytes_per_pixel = 4;

  Image_buffer image = {};
  {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = d3d->context->Map((ID3D11Resource*)copy_texture, 0, D3D11_MAP_READ, 0, &mapped);
    {
      U64 size_for_image = texture_height * texture_width * bytes_per_pixel;
      image.bytes_per_pixel = bytes_per_pixel;
      // image.row_stride      = (U64)mapped.RowPitch; 
      image.width_in_px     = texture_width;
      image.height_in_px    = texture_height;
      image.data            = ArenaPushArr(arena, U8, size_for_image);

      HandleLater(hr == S_OK);
      if (mapped.pData)
      {
        for (U64 row_index = 0; row_index < texture_height; row_index += 1)
        {
          memcpy(
            image.data + row_index * texture_width * bytes_per_pixel, 
            (U8*)mapped.pData + row_index * mapped.RowPitch,
            texture_width * bytes_per_pixel
          );
        }
      }
    }
    d3d->context->Unmap((ID3D11Resource*)copy_texture, 0);
  }

  copy_texture->Release();
  end_scratch(&scratch);

  return image;
}



#endif