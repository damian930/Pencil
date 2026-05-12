#ifndef OS_WIN32_CPP
#define OS_WIN32_CPP 

#include "os/win32.h"
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "oneCore.lib")

struct OS_Window {
  WNDCLASSEXA window_class;
  HWND handle;

  B32 should_close;

  // Per frame data
  V2F32 dims;
  V2F32 client_area_dims;
};

struct OS_State {
  Arena* state_arena;
  
  HINSTANCE app_instance;
  U64 perf_freq_count_per_sec;
  Str8 path_to_system_fonts;

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

global OS_State* __os_g_state = 0;

///////////////////////////////////////////////////////////
// - State
//
void os_init()
{
  Arena* arena = arena_alloc(Megabytes(64));
  __os_g_state = ArenaPush(arena, OS_State);
  __os_g_state->state_arena = arena; 

  // Performance frequency
  LARGE_INTEGER freq_lr = {};
  BOOL succ = QueryPerformanceFrequency(&freq_lr); Assert(succ);
  __os_g_state->perf_freq_count_per_sec = freq_lr.QuadPart;

  // Setting keys
  StaticAssert(ArrayCount(__os_g_state->key_states) == Key__COUNT);
  for (U64 i = 0; i < Key__COUNT; i += 1) 
  {
    OS_Key_state* key_state = __os_g_state->key_states + i;
    key_state->key = (Key)((U32)Key__NONE + i);
    key_state->is_up = true;
  }

  // Setting mouse buttons
  StaticAssert(ArrayCount(__os_g_state->mouse_button_states) == Mouse_button__COUNT);
  for (U64 i = 0; i < Mouse_button__COUNT; i += 1) 
  {
    OS_Mouse_button_state* button_state = __os_g_state->mouse_button_states + i;
    button_state->button = (Mouse_button)((U32)Mouse_button__NONE + i);
    button_state->is_up = true;
  }

  // Getting the folder path in win32
  {
    WCHAR* wstr = 0;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, 0, &wstr);
    Handle(hr == S_OK);
    Str8 str = str8_from_wstr(__os_g_state->state_arena, wstr);
    __os_g_state->path_to_system_fonts = str;
  }
}

void os_release()
{
  InvalidCodePath();
}

OS_State* os_get_state()
{
  return __os_g_state;
}

///////////////////////////////////////////////////////////
// - Files
//
OS_File os_file_handle_zero()
{
  OS_File file = {};
  file.u64 = (U64)INVALID_HANDLE_VALUE;
  return file;
}

B32 os_file_handle_match(OS_File handle, OS_File other)
{
  return handle.u64 == other.u64;
}

B32 os_file_is_valid(OS_File file)
{
  return !os_file_handle_match(file, os_file_handle_zero());
}

OS_File_props os_file_get_props(OS_File file)
{
  if (os_file_handle_match(file, OS_File{})) { return {}; }

  OS_File_props result_properties = {};
  BY_HANDLE_FILE_INFORMATION file_info = {};
  result_properties.succ = GetFileInformationByHandle((HANDLE)file.u64, &file_info);
  if (result_properties.succ)
  {
    result_properties.size = u64_from_2_u32(file_info.nFileSizeHigh, file_info.nFileSizeLow);
  }
  return result_properties;
}

OS_File os_file_open_ex(Str8 file_name, OS_File_access_flags acess_flags, OS_Error* out_error)
{
  /* todo:
    - Max path for file path and its extention to 32000
  */
  Scratch scratch = get_scratch(0, 0);
  Str8 fil_name_nt = str8_copy_alloc(scratch.arena, file_name);

  // todo: See what happends if i dont have open existing as default, 
  //       will i then just be able to check if file exists easily
  
  U32 desired_access = 0; 
  U32 share_mode = 0; 
  U32 creation_dispostion = OPEN_EXISTING; 

  // This case would be fine, but just wouldnt make much sense, so asserting
  if (acess_flags & OS_File_access__read) { desired_access |= GENERIC_READ; }
  if (acess_flags & OS_File_access__write) { desired_access |= GENERIC_WRITE; }
  if (acess_flags & OS_File_access__append) { desired_access |= GENERIC_WRITE; }

  if (acess_flags & OS_File_access__read) { share_mode |= FILE_SHARE_READ; }
  if (acess_flags & OS_File_access__share_read) { share_mode |= FILE_SHARE_READ; }
  if (acess_flags & OS_File_access__share_write) { share_mode |= FILE_SHARE_WRITE; }

  if (acess_flags & OS_File_access__read) { creation_dispostion = OPEN_EXISTING; }
  if (acess_flags & OS_File_access__write) { creation_dispostion = CREATE_ALWAYS; }
  if (acess_flags & OS_File_access__append) { creation_dispostion = OPEN_ALWAYS; }

  HANDLE file_handle = CreateFileA(
    (char*)fil_name_nt.data,
    desired_access,
    share_mode,
    Null,
    creation_dispostion,
    Null, 
    Null  // Some template file or something
  );
  U32 error_code = GetLastError();
  if (error_code == ERROR_ACCESS_DENIED && out_error) {
    *out_error = OS_Error__access_denied;
  } else if (error_code == ERROR_FILE_NOT_FOUND && out_error) {
    *out_error = OS_Error__no_such_path;
  } else if (error_code == ERROR_ALREADY_EXISTS && out_error) {
    *out_error = OS_Error__already_exists;
  }

  end_scratch(&scratch);

  // note: We use 0 to be able to creata OS_File file = {}, instead of having to do this 
  //       every time OS_File file = os_file_zero(). Win32 return INVALID_HANDLE on fail to open file,
  //       but it also seems to never give out handles with value of 0. 
  //       If at some point i realise that win32 does use 0 for valid handles, i will 
  //       change the api.  

  OS_File file = {};
  file.u64 = (U64)file_handle; // note: file handle is INVALID_HANDLE_VALUE if we failed to open the file
  return file;
}

OS_File os_file_open(Str8 file_name, OS_File_access_flags acess_flags)
{
  OS_File file = os_file_open_ex(file_name, acess_flags, 0);
  return file;
}

void os_file_close(OS_File* file)
{
  if (os_file_handle_match(OS_File{}, *file)) { return; }
  (void)CloseHandle((HANDLE)file->u64);
  *file = OS_File{};
}

// note: Returns Str8{} if fails, though i shoud not fail unless there is a bug in the implementation
Str8 os_get_current_dir_path(Arena* arena)
{
  U64 arena_pos_before_allocations = arena_get_pos(arena);

  B32 failed = false;
  Str8 result_path = {};
  do {
    U32 path_buffer_size_with_nt = GetCurrentDirectoryA(0, 0);
    if (path_buffer_size_with_nt == 0) { failed = true; break; }
    
    Data_buffer path_buffer = data_buffer_make(arena, path_buffer_size_with_nt);
    U32 bytes_written_no_nt = GetCurrentDirectoryA(path_buffer_size_with_nt, (char*)path_buffer.data);
    if (bytes_written_no_nt != path_buffer_size_with_nt - 1) { failed = true; break; }
    
    result_path = str8_front(path_buffer, bytes_written_no_nt);
    if (result_path.data[result_path.count] != '\0') { failed = true; break; }
    arena_pop(arena, 1); // Popping '\0;
  } while (0);
  
  if (failed) {
    result_path = Str8{};
    arena_pop_to_pos(arena, arena_pos_before_allocations);
  }
  return result_path;
}

// todo: now its b32 here, cause i have no idea what i need to check, so just b32 this bitch
B32 os_file_read(OS_File file, Data_buffer* out_buffer)
{
  if (!os_file_is_valid(file)) { return false; }

  // note:
  // I really dont have an idea for this api yet. Right now all i need is just
  // regular reads into a buffer that is always the same size as the file, 
  // i have never read a file otherwise, so for now just that. When i have a case
  // where i need to read it another way i will see what i need and change the api,
  // right now trying to come up with a better api i just wasting time. 
  Assert(os_file_get_props(file).size == out_buffer->count);

  LARGE_INTEGER new_pointer_pos = {};
  LARGE_INTEGER distance_to_move = {};
  distance_to_move.QuadPart = 0;
  
  B32 succ = false;

  if (SetFilePointerEx((HANDLE)file.u64, distance_to_move, &new_pointer_pos, FILE_BEGIN))
  {
    succ = true;

    U64 bytes_read = 0;
    for (;bytes_read < out_buffer->count;)
    {
      // todo: This will just rewrite the data in the out buffer when we do the second loop, fix this
      U32 bytes_read_this_time = 0;
      BOOL read_succ = ReadFile(
        (HANDLE)file.u64, 
        (void*)(out_buffer->data + bytes_read), 
        (U32)(out_buffer->count - bytes_read), 
        (DWORD*)&bytes_read_this_time, 
        Null
      );
      bytes_read += bytes_read_this_time;
  
      if (!read_succ) { succ = false; break; }
    }
  }

  return succ; 
}

B32 os_file_write_end(OS_File file, Data_buffer buffer)
{
  if (!os_file_is_valid(file)) { return false; }
  
  B32 succ = true;
  
  LARGE_INTEGER new_pointer_pos = {};
  if (SetFilePointerEx((HANDLE)file.u64, LARGE_INTEGER{}, &new_pointer_pos, FILE_END))
  {
    // todo: Make sure this doesnt fail of buffer longer then U32 holds
    U64 bytes_written = 0;
    for (;bytes_written < buffer.count;)
    {
      U32 bytes_written_this_time = 0;
      U64 bytes_to_write = buffer.count - bytes_written;
      BOOL write_succ = WriteFile((HANDLE)file.u64, buffer.data + bytes_written, 
                                  (U32)bytes_to_write, 
                                  (DWORD*)&bytes_written_this_time, Null);
      if (!write_succ) { succ = false; break; }
      bytes_written += (U64)bytes_written_this_time;
    }
  } else { succ = false; }

  return succ;
}

///////////////////////////////////////////////////////////
// - Frame
//
void os_frame_begin()
{
  OS_State* os_state = os_get_state();
  
  // Setting prev frame's input data to the new frame's fresh input data
  {
    StaticAssert(ArrayCount(os_state->key_states) == Key__COUNT);
    for (U64 i = 0; i < Key__COUNT; i += 1) 
    {
      OS_Key_state* key_state = __os_g_state->key_states + i;
      key_state->was_down = key_state->is_down;
      key_state->was_up = key_state->is_up;
    }

    // Setting mouse buttons
    StaticAssert(ArrayCount(os_state->mouse_button_states) == Mouse_button__COUNT);
    for (U64 i = 0; i < Mouse_button__COUNT; i += 1) 
    {
      OS_Mouse_button_state* button_state = os_state->mouse_button_states + i;
      button_state->was_down = button_state->is_down;
      button_state->was_up = button_state->is_up;
    }
  }

  // Creating frame events though the winproc
  for (MSG msg = {}; PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
  
  // Mouse positions
  os_state->prev_frame_mouse_pos = os_state->this_frame_mouse_pos;
  {
    POINT p = {};
    BOOL succ = {};
    succ = GetCursorPos(&p); Assert(succ);
    succ = ScreenToClient(os_get_state()->window.handle, &p); Assert(succ);
    Handle(succ); 
    os_state->this_frame_mouse_pos = v2s32(p.x, p.y);
  }

  // Window dims
  {
    RECT rect = {};
    BOOL succ = GetWindowRect(os_state->window.handle, &rect); Assert(succ);
    V2F32 dims = v2f32((F32)(rect.right - rect.left), (F32)(rect.bottom - rect.top));
    os_state->window.dims = dims;
  }

  // Window client arena dims
  {
    RECT rect = {};
    BOOL succ = GetClientRect(os_state->window.handle, &rect); Assert(succ);
    V2F32 dims = v2f32((F32)(rect.right - rect.left), (F32)(rect.bottom - rect.top));
    os_state->window.client_area_dims = dims;
  }
}

void os_frame_end() {}

///////////////////////////////////////////////////////////
// - Windowing
V2F32 os_get_window_dims()
{
  OS_State* os_state = os_get_state();
  return os_state->window.dims;
}

V2F32 os_get_client_area_dims()
{
  OS_State* os_state = os_get_state();
  return os_state->window.client_area_dims;
}

V2F32 os_get_client_area_dims__unsynched()
{
  Handle(true); // todo: I just dont like this func here. I only added it to be able to create a swap chain before i start a frame, maybe i shoud create a swap chain inside the first ever call to render_begin instead of having to have it in the init call ???
  RECT rect = {};
  BOOL succ = GetWindowRect(os_get_state()->window.handle, &rect); Assert(succ);
  V2F32 dims = v2f32((F32)(rect.right - rect.left), (F32)(rect.bottom - rect.top));
  return dims;
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
  return os_get_state()->window.should_close;
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

// todo: Maybe this shoud be a macro instead of a call
F64 os_get_time_for_timing_sec()
{
  return (F64)os_get_perf_counter() / (F64)os_get_perf_freq_per_sec(); 
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
          case VK_DELETE:  { key = Key__Delete;  } break;
          case VK_TAB:     { key = Key__Tab;     } break;
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
      win32_state->window.should_close = true;
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

Str8 os_get_path_to_system_fonts()
{
  return os_get_state()->path_to_system_fonts;
}

Str8 str8_from_wstr(Arena* arena, WCHAR* wstr)
{
  U64 str8_len_with_nt = WideCharToMultiByte(CP_UTF8, Null, wstr, -1, 0, 0, Null, Null);
  Str8 str = data_buffer_make(arena, str8_len_with_nt);
  U64 succ = WideCharToMultiByte(CP_UTF8, Null, wstr, -1, (char*)str.data, (int)str.count, Null, Null);
  InvariantCheck(succ); 
  InvariantCheck(str.data[str.count - 1] == '\0');
  str = str8_chop_back_if_match(str, Str8FromC("\0"), 0);
  return str;
}


#endif