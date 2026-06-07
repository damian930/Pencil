#ifndef OS_WIN32_CPP
#define OS_WIN32_CPP 

#include "os/win32.h"
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "oneCore.lib")

struct OS_Window {
  WNDCLASSEXA window_class;
  HWND handle;

  // Has to be specified at window creation and cant be changed later
  B32 is_transparent;

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
  Arena* frame_arena;
  //
  OS_Event_list frame_event_list;
  //
  V2F32 this_frame_mouse_pos; // Relative to the client area of the window
  V2F32 prev_frame_mouse_pos; // Relative to the client area of the window
  // 
  F32 start_time_for_prev_frame;
  F32 start_time_for_this_frame;
};

// These are used for alloations. We need allocation to allocat the arena for the os layer.
// For that reason we store them here and not inside the os layer state data.
global U64 __os_g_allocation_granularity = Kilobytes(64);
global U64 __os_g_page_size              = Kilobytes(4);
global OS_State* __os_g_state            = 0;

///////////////////////////////////////////////////////////
// - State
//
void os_init()
{
  { // Getting things needed for allocations that we need to create state
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);
    __os_g_allocation_granularity = (U64)info.dwAllocationGranularity;
    __os_g_page_size              = (U64)info.dwPageSize;
  }

  // Bootstraping the page size from the os to arena
  __arena_g_page_size = __os_g_page_size;

  Arena* arena = arena_alloc(Kilobytes(64), false, 0);
  __os_g_state = ArenaPush(arena, OS_State);
  __os_g_state->state_arena = arena; 
  
  // Performance frequency
  LARGE_INTEGER freq_lr = {};
  BOOL succ = QueryPerformanceFrequency(&freq_lr); Assert(succ);
  __os_g_state->perf_freq_count_per_sec = freq_lr.QuadPart;

  // Some system info
  {
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);
  }

  // Getting the folder path in win32 to system fonts
  {
    WCHAR* wstr = 0;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, 0, &wstr);
    Handle(hr == S_OK);
    Str8 str = str8_from_wstr(__os_g_state->state_arena, wstr);
    __os_g_state->path_to_system_fonts = str;
  }
 
  __os_g_state->frame_arena = arena_alloc(Kilobytes(64), false, 0);
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
  Handle(os_file_get_props(file).size == out_buffer->count);

  LARGE_INTEGER new_pointer_pos  = {};
  LARGE_INTEGER distance_to_move = {};
  distance_to_move.QuadPart = 0;
  
  B32 succ = false;

  if (SetFilePointerEx((HANDLE)file.u64, distance_to_move, &new_pointer_pos, FILE_BEGIN))
  {
    succ = true;

    U64 bytes_read = 0;
    for (;bytes_read < out_buffer->count;)
    {
      U32 bytes_to_read = (U32)clamp_u64(out_buffer->count - bytes_read, 0, u32_max);

      // todo: This will just rewrite the data in the out buffer when we do the second loop, fix this
      U32 bytes_read_this_time = 0;
      BOOL read_succ = ReadFile((HANDLE)file.u64, 
                                (void*)(out_buffer->data + bytes_read), 
                                bytes_to_read, 
                                (DWORD*)&bytes_read_this_time, 
                                Null);
                                
      if (!read_succ || bytes_read_this_time != bytes_to_read) { BP; succ = false; break; }
      bytes_read += bytes_read_this_time;
    }
  }

  return succ; 
}

B32 os_file_write_end(OS_File file, Data_buffer buffer)
{
  if (!os_file_is_valid(file)) { return false; }
  
  B32 succ = false;
  
  LARGE_INTEGER new_pointer_pos = {};
  if (SetFilePointerEx((HANDLE)file.u64, LARGE_INTEGER{}, &new_pointer_pos, FILE_END))
  {
    succ = true;

    U64 bytes_written = 0;
    for (;bytes_written < buffer.count;)
    {
      U32 bytes_written_this_time = 0;
      U32 bytes_to_write          = (U32)clamp_u64(buffer.count - bytes_written, 0, u32_max);
      BOOL write_succ             = WriteFile((HANDLE)file.u64, buffer.data + bytes_written, 
                                              bytes_to_write, 
                                              (DWORD*)&bytes_written_this_time, Null);
      if (!write_succ || bytes_to_write != bytes_written_this_time) { succ = false; break; }
      bytes_written += (U64)bytes_written_this_time;
    }
  } else { succ = false; }

  return succ;
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

///////////////////////////////////////////////////////////
// - Memory
//
Mem_chunk os_reserve_mem_chunk(U64 n_pages, B32 start_at_specific_page, U32 allocation_granulatity_index)
{
  OS_State* os_state = os_get_state();
  
  // todo: If i am not mistacke, we waste some space here by not checking if we reserve more than pages since
  //       ganularity might be a couple of pages
  void* v_alloc_addr = Null;
  if (start_at_specific_page) {
    v_alloc_addr = (void*)((allocation_granulatity_index + 1) * __os_g_allocation_granularity);
  }
  U64 bytes_to_reserve = n_pages * __os_g_page_size;
  
  void* mem = VirtualAlloc(v_alloc_addr, bytes_to_reserve, MEM_RESERVE, PAGE_NOACCESS);
  
  Mem_chunk result_mem_chunk = {};
  {
    MEMORY_BASIC_INFORMATION mem_info = {};
    U64 vq_result = VirtualQuery(mem, &mem_info, sizeof(mem_info));
    if (vq_result == sizeof(mem_info))
    {
      if (mem_info.RegionSize == bytes_to_reserve) {
        result_mem_chunk.base_p           = mem;
        result_mem_chunk.n_pages_reserved = n_pages;
      }
    }
  }
  return result_mem_chunk;
}

B32 os_commit_mem_pages_to_chunk(Mem_chunk* mem_chunk, U64 n_pages)
{
  if (mem_chunk == 0) { InvalidCodePath(); return false; }
  if (mem_chunk->n_pages_reserved == mem_chunk->n_pages_commited) { return false; }

  OS_State* os_state = os_get_state();

  U64 pages_left_to_commit = mem_chunk->n_pages_reserved - mem_chunk->n_pages_commited;
  if (n_pages > pages_left_to_commit) { InvalidCodePath(); n_pages = pages_left_to_commit; }

  U64 bytes_to_commit = (mem_chunk->n_pages_commited + n_pages) * __os_g_page_size; 
  void* mem = VirtualAlloc(mem_chunk->base_p, bytes_to_commit, MEM_COMMIT, PAGE_READWRITE);

  B32 succ = false;
  if (mem != 0) // Failed to commit 
  {
    MEMORY_BASIC_INFORMATION mem_info = {};
    U64 vq_result = VirtualQuery(mem_chunk->base_p, &mem_info, sizeof(mem_info));
    if (vq_result == sizeof(mem_info)) 
    {
      if (mem_info.RegionSize == ((mem_chunk->n_pages_commited + n_pages) * __os_g_page_size)) {
        succ = true;
        mem_chunk->n_pages_commited += n_pages;
      } 
    } 
  } 
  return succ;
}

B32 os_decommit_mem_pages_from_chuck(Mem_chunk* mem_chunk, U64 n_pages)
{
  if (mem_chunk == 0) { InvalidCodePath(); return false; }
  if (mem_chunk->n_pages_commited == 0) { InvalidCodePath(); return false; }

  OS_State* os_state = os_get_state();
  
  if (n_pages > mem_chunk->n_pages_commited) { n_pages = mem_chunk->n_pages_commited; }

  U64 n_pages_left_commited_after_decommition = mem_chunk->n_pages_commited - n_pages;
  void* base_p_for_decommit_range             = (void*)((U64)mem_chunk->base_p + (n_pages_left_commited_after_decommition * __os_g_page_size));
  U64 bytes_to_decommit                       = n_pages * __os_g_page_size; 
  BOOL decommit_succ                          = VirtualFree(base_p_for_decommit_range, bytes_to_decommit, MEM_DECOMMIT);

  if (decommit_succ) {
    mem_chunk->n_pages_commited -= n_pages;
  }
  return decommit_succ;
}

B32 os_release_mem_chunk(Mem_chunk* mem_chunk)
{
  if (mem_chunk == 0) { InvalidCodePath(); return false; }

  OS_State* os_state = os_get_state();
  B32 release_succ = VirtualFree(mem_chunk->base_p, 0, MEM_RELEASE);
  if (release_succ) {
    *mem_chunk = Mem_chunk{};
  }
  return release_succ;
}

U64 os_get_mem_page_size()
{
  return __os_g_page_size;
}

// // Defines for core/Arena
// #define __ArenaReserveMemChunk(n_pages, start_at_specific_page, allocation_granulatity_index) \
//     os_reserve_mem_chunk((n_pages), (start_at_specific_page), (allocation_granulatity_index))
// #define __ArenaCommitMemPagesToChunk(mem_chunk, n_pages) \
//     os_commit_mem_pages_to_chunk((mem_chunk), (n_pages))
// #define __ArenaDecommitMemPagesFromChunk(mem_chunk, n_pages) \
//     os_decommit_mem_pages_from_chuck((mem_chunk), (n_pages))
// #define __ArenaReleaseMemChunk(mem_chunk) \
//     os_release_mem_chunk((mem_chunk))

///////////////////////////////////////////////////////////
// - Frame
//
void os_frame_begin()
{
  OS_State* os_state = os_get_state();
  
  // Resetting data from the prev frame
  {
    os_state->frame_event_list     = OS_Event_list{};
    os_state->prev_frame_mouse_pos = os_state->this_frame_mouse_pos;
    os_state->this_frame_mouse_pos = v2f32(f32_neg_inf(), f32_neg_inf());
    arena_clear(os_state->frame_arena);
  }

  // Creating frame events though the winproc
  for (MSG msg = {}; PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  // Capturing the mouse when if pressed 
  {
    static B32 mouse_button_is_down_states[Mouse_button__COUNT] = {};

    for (OS_Event* ev = os_state->frame_event_list.first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.went_down) { mouse_button_is_down_states[ev->mouse_event.button] = true; }
      if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.went_up) { mouse_button_is_down_states[ev->mouse_event.button] = false; }
    }

    B32 some_button_is_down = false;
    for EachEnumRange(button, Mouse_button, Mouse_button__NONE, Mouse_button__COUNT)
    {
      if (mouse_button_is_down_states[button]) {
        some_button_is_down = true;
        break;
      }
    }

    HWND widnow_that_is_capturing_the_mouse = GetCapture();
    if (os_state->window.handle != widnow_that_is_capturing_the_mouse && some_button_is_down) {
      SetCapture(os_state->window.handle);
    } else if (os_state->window.handle == widnow_that_is_capturing_the_mouse && !some_button_is_down) {
      ReleaseCapture();
    }
  }

  // Mouse positions
  {
    POINT p = {};
    BOOL succ = {};
    succ |= GetCursorPos(&p); Assert(succ);
    succ |= ScreenToClient(os_get_state()->window.handle, &p); Assert(succ);
    Handle(succ); 
    os_state->this_frame_mouse_pos = v2f32((F32)p.x, (F32)p.y);
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

  // Getting frame time 
  {
    os_state->start_time_for_prev_frame = os_state->start_time_for_this_frame;
    os_state->start_time_for_this_frame = (F32)os_get_time_for_timing_sec();
  }
}

void os_frame_end() {}

F32 os_get_time_since_last_frame()
{
  OS_State* os_state = os_get_state();
  F32 diff = os_state->start_time_for_this_frame - os_state->start_time_for_prev_frame;
  return diff;
}

OS_Event_list* os_get_frame_event_list()
{
  return &os_get_state()->frame_event_list;
}

void os_consume_frame_event(OS_Event* event)
{
  // todo: I dont like that the event here might not even be a part of the event, bad api i think
  B32 found = false;
  for (OS_Event* test_event = os_get_state()->frame_event_list.first; test_event; test_event = test_event->next)
  {
    if (test_event == event) { found = true; break; }
  }
  
  DllPop(&os_get_state()->frame_event_list, event);
  if (found) {
    os_get_state()->frame_event_list.count -= 1;
  } else {
    InvalidCodePath("What a bad api for real");
  }
}

///////////////////////////////////////////////////////////
// - Windowing
//
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
  return os_get_state()->this_frame_mouse_pos;
}

V2F32 os_get_prev_mouse_pos()
{
  return os_get_state()->prev_frame_mouse_pos;
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

B32 os_window_is_transparent()
{
  return os_get_state()->window.is_transparent;
}

///////////////////////////////////////////////////////////
// - Key stuff 
//
Key key_from_str8(Str8 str)
{
  Key result_key = Key__NONE;
  
  str = str8_trim(str);
  if (0) {}

  // Mods
  else if (str8_match(Str8FromC("shift"), str, Str8_match__ignore_case))   { result_key = Key__shift; }
  else if (str8_match(Str8FromC("control"), str, Str8_match__ignore_case)) { result_key = Key__control; }
  else if (str8_match(Str8FromC("alt"), str, Str8_match__ignore_case))     { result_key = Key__alt; }

  // Letters
  else if (str8_match(Str8FromC("a"), str, Str8_match__ignore_case)) { result_key = Key__a; }
  else if (str8_match(Str8FromC("b"), str, Str8_match__ignore_case)) { result_key = Key__b; }
  else if (str8_match(Str8FromC("c"), str, Str8_match__ignore_case)) { result_key = Key__c; }
  else if (str8_match(Str8FromC("d"), str, Str8_match__ignore_case)) { result_key = Key__d; }
  else if (str8_match(Str8FromC("e"), str, Str8_match__ignore_case)) { result_key = Key__e; }
  else if (str8_match(Str8FromC("f"), str, Str8_match__ignore_case)) { result_key = Key__f; }
  else if (str8_match(Str8FromC("g"), str, Str8_match__ignore_case)) { result_key = Key__g; }
  else if (str8_match(Str8FromC("h"), str, Str8_match__ignore_case)) { result_key = Key__h; }
  else if (str8_match(Str8FromC("i"), str, Str8_match__ignore_case)) { result_key = Key__i; }
  else if (str8_match(Str8FromC("j"), str, Str8_match__ignore_case)) { result_key = Key__j; }
  else if (str8_match(Str8FromC("k"), str, Str8_match__ignore_case)) { result_key = Key__k; }
  else if (str8_match(Str8FromC("l"), str, Str8_match__ignore_case)) { result_key = Key__l; }
  else if (str8_match(Str8FromC("m"), str, Str8_match__ignore_case)) { result_key = Key__m; }
  else if (str8_match(Str8FromC("n"), str, Str8_match__ignore_case)) { result_key = Key__n; }
  else if (str8_match(Str8FromC("o"), str, Str8_match__ignore_case)) { result_key = Key__o; }
  else if (str8_match(Str8FromC("p"), str, Str8_match__ignore_case)) { result_key = Key__p; }
  else if (str8_match(Str8FromC("q"), str, Str8_match__ignore_case)) { result_key = Key__q; }
  else if (str8_match(Str8FromC("r"), str, Str8_match__ignore_case)) { result_key = Key__r; }
  else if (str8_match(Str8FromC("s"), str, Str8_match__ignore_case)) { result_key = Key__s; }
  else if (str8_match(Str8FromC("t"), str, Str8_match__ignore_case)) { result_key = Key__t; }
  else if (str8_match(Str8FromC("u"), str, Str8_match__ignore_case)) { result_key = Key__u; }
  else if (str8_match(Str8FromC("v"), str, Str8_match__ignore_case)) { result_key = Key__v; }
  else if (str8_match(Str8FromC("w"), str, Str8_match__ignore_case)) { result_key = Key__w; }
  else if (str8_match(Str8FromC("x"), str, Str8_match__ignore_case)) { result_key = Key__x; }
  else if (str8_match(Str8FromC("y"), str, Str8_match__ignore_case)) { result_key = Key__y; }
  else if (str8_match(Str8FromC("z"), str, Str8_match__ignore_case)) { result_key = Key__z; }

  // Numbers
  else if (str8_match(Str8FromC("0"), str, Str8_match__ignore_case)) { result_key = Key__0; }
  else if (str8_match(Str8FromC("1"), str, Str8_match__ignore_case)) { result_key = Key__1; }
  else if (str8_match(Str8FromC("2"), str, Str8_match__ignore_case)) { result_key = Key__2; }
  else if (str8_match(Str8FromC("3"), str, Str8_match__ignore_case)) { result_key = Key__3; }
  else if (str8_match(Str8FromC("4"), str, Str8_match__ignore_case)) { result_key = Key__4; }
  else if (str8_match(Str8FromC("5"), str, Str8_match__ignore_case)) { result_key = Key__5; }
  else if (str8_match(Str8FromC("6"), str, Str8_match__ignore_case)) { result_key = Key__6; }
  else if (str8_match(Str8FromC("7"), str, Str8_match__ignore_case)) { result_key = Key__7; }
  else if (str8_match(Str8FromC("8"), str, Str8_match__ignore_case)) { result_key = Key__8; }
  else if (str8_match(Str8FromC("9"), str, Str8_match__ignore_case)) { result_key = Key__9; }

  // Printable
  else if (str8_match(Str8FromC("space"), str, Str8_match__ignore_case))           { result_key = Key__space; }
  else if (str8_match(Str8FromC("backtick"), str, Str8_match__ignore_case))        { result_key = Key__backtick; }
  else if (str8_match(Str8FromC("minus"), str, Str8_match__ignore_case))           { result_key = Key__minus; }
  else if (str8_match(Str8FromC("equals"), str, Str8_match__ignore_case))          { result_key = Key__equals; }
  else if (str8_match(Str8FromC("left_bracket"), str, Str8_match__ignore_case))    { result_key = Key__left_bracket; }
  else if (str8_match(Str8FromC("right_bracket"), str, Str8_match__ignore_case))   { result_key = Key__right_bracket; }
  else if (str8_match(Str8FromC("backslash"), str, Str8_match__ignore_case))       { result_key = Key__backslash; }
  else if (str8_match(Str8FromC("semicolon"), str, Str8_match__ignore_case))       { result_key = Key__semicolon; }
  else if (str8_match(Str8FromC("apostrophe"), str, Str8_match__ignore_case))      { result_key = Key__apostrophe; }
  else if (str8_match(Str8FromC("comma"), str, Str8_match__ignore_case))           { result_key = Key__comma; }
  else if (str8_match(Str8FromC("period"), str, Str8_match__ignore_case))          { result_key = Key__period; }
  else if (str8_match(Str8FromC("slash"), str, Str8_match__ignore_case))           { result_key = Key__slash; }

  // Special
  else if (str8_match(Str8FromC("left_arrow"), str, Str8_match__ignore_case)) { result_key = Key__left_arrow; }
  else if (str8_match(Str8FromC("right_arrow"), str, Str8_match__ignore_case)) { result_key = Key__right_arrow; }
  else if (str8_match(Str8FromC("up_arrow"), str, Str8_match__ignore_case)) { result_key = Key__up_arrow; }
  else if (str8_match(Str8FromC("down_arrow"), str, Str8_match__ignore_case)) { result_key = Key__down_arrow; }
  else if (str8_match(Str8FromC("home"), str, Str8_match__ignore_case)) { result_key = Key__home; }
  else if (str8_match(Str8FromC("end"), str, Str8_match__ignore_case)) { result_key = Key__end; }
  else if (str8_match(Str8FromC("page_up"), str, Str8_match__ignore_case)) { result_key = Key__page_up; }
  else if (str8_match(Str8FromC("page_down"), str, Str8_match__ignore_case)) { result_key = Key__page_down; }
  else if (str8_match(Str8FromC("backspace"), str, Str8_match__ignore_case)) { result_key = Key__backspace; }
  else if (str8_match(Str8FromC("delete"), str, Str8_match__ignore_case)) { result_key = Key__delete; }
  else if (str8_match(Str8FromC("insert"), str, Str8_match__ignore_case)) { result_key = Key__insert; }
  else if (str8_match(Str8FromC("escape"), str, Str8_match__ignore_case)) { result_key = Key__escape; }
  else if (str8_match(Str8FromC("tab"), str, Str8_match__ignore_case)) { result_key = Key__tab; }
  else if (str8_match(Str8FromC("enter"), str, Str8_match__ignore_case)) { result_key = Key__enter; }
  else if (str8_match(Str8FromC("caps_lock"), str, Str8_match__ignore_case)) { result_key = Key__caps_lock; }

  return result_key;
}

Str8 str8_from_key(Key key)
{
  Str8 result = {};

  if (0) {}

  // Mods
  else if (key == Key__shift)   { result = Str8FromC("shift"); }
  else if (key == Key__control) { result = Str8FromC("control"); }
  else if (key == Key__alt)     { result = Str8FromC("alt"); }

  // Letters
  else if (key == Key__a) { result = Str8FromC("a"); }
  else if (key == Key__b) { result = Str8FromC("b"); }
  else if (key == Key__c) { result = Str8FromC("c"); }
  else if (key == Key__d) { result = Str8FromC("d"); }
  else if (key == Key__e) { result = Str8FromC("e"); }
  else if (key == Key__f) { result = Str8FromC("f"); }
  else if (key == Key__g) { result = Str8FromC("g"); }
  else if (key == Key__h) { result = Str8FromC("h"); }
  else if (key == Key__i) { result = Str8FromC("i"); }
  else if (key == Key__j) { result = Str8FromC("j"); }
  else if (key == Key__k) { result = Str8FromC("k"); }
  else if (key == Key__l) { result = Str8FromC("l"); }
  else if (key == Key__m) { result = Str8FromC("m"); }
  else if (key == Key__n) { result = Str8FromC("n"); }
  else if (key == Key__o) { result = Str8FromC("o"); }
  else if (key == Key__p) { result = Str8FromC("p"); }
  else if (key == Key__q) { result = Str8FromC("q"); }
  else if (key == Key__r) { result = Str8FromC("r"); }
  else if (key == Key__s) { result = Str8FromC("s"); }
  else if (key == Key__t) { result = Str8FromC("t"); }
  else if (key == Key__u) { result = Str8FromC("u"); }
  else if (key == Key__v) { result = Str8FromC("v"); }
  else if (key == Key__w) { result = Str8FromC("w"); }
  else if (key == Key__x) { result = Str8FromC("x"); }
  else if (key == Key__y) { result = Str8FromC("y"); }
  else if (key == Key__z) { result = Str8FromC("z"); }

  // Numbers
  else if (key == Key__0) { result = Str8FromC("0"); }
  else if (key == Key__1) { result = Str8FromC("1"); }
  else if (key == Key__2) { result = Str8FromC("2"); }
  else if (key == Key__3) { result = Str8FromC("3"); }
  else if (key == Key__4) { result = Str8FromC("4"); }
  else if (key == Key__5) { result = Str8FromC("5"); }
  else if (key == Key__6) { result = Str8FromC("6"); }
  else if (key == Key__7) { result = Str8FromC("7"); }
  else if (key == Key__8) { result = Str8FromC("8"); }
  else if (key == Key__9) { result = Str8FromC("9"); }

  // Printable
  else if (key == Key__space)         { result = Str8FromC("space"); }
  else if (key == Key__backtick)      { result = Str8FromC("backtick"); }
  else if (key == Key__minus)         { result = Str8FromC("minus"); }
  else if (key == Key__equals)        { result = Str8FromC("equals"); }
  else if (key == Key__left_bracket)  { result = Str8FromC("left_bracket"); }
  else if (key == Key__right_bracket) { result = Str8FromC("right_bracket"); }
  else if (key == Key__backslash)     { result = Str8FromC("backslash"); }
  else if (key == Key__semicolon)     { result = Str8FromC("semicolon"); }
  else if (key == Key__apostrophe)    { result = Str8FromC("apostrophe"); }
  else if (key == Key__comma)         { result = Str8FromC("comma"); }
  else if (key == Key__period)        { result = Str8FromC("period"); }
  else if (key == Key__slash)         { result = Str8FromC("slash"); }

  // Special
  else if (key == Key__left_arrow)  { result = Str8FromC("left_arrow"); }
  else if (key == Key__right_arrow) { result = Str8FromC("right_arrow"); }
  else if (key == Key__up_arrow)    { result = Str8FromC("up_arrow"); }
  else if (key == Key__down_arrow)  { result = Str8FromC("down_arrow"); }
  else if (key == Key__home)        { result = Str8FromC("home"); }
  else if (key == Key__end)         { result = Str8FromC("end"); }
  else if (key == Key__page_up)     { result = Str8FromC("page_up"); }
  else if (key == Key__page_down)   { result = Str8FromC("page_down"); }
  else if (key == Key__backspace)   { result = Str8FromC("backspace"); }
  else if (key == Key__delete)      { result = Str8FromC("delete"); }
  else if (key == Key__insert)      { result = Str8FromC("insert"); }
  else if (key == Key__escape)      { result = Str8FromC("escape"); }
  else if (key == Key__tab)         { result = Str8FromC("tab"); }
  else if (key == Key__enter)       { result = Str8FromC("enter"); }
  else if (key == Key__caps_lock)   { result = Str8FromC("caps_lock"); }

  return result;
}

OS_Event_modifier os_modifier_from_key(Key key)
{
  OS_Event_modifier mod = OS_Event_modifier__NONE;
  switch (key)
  {
    default: {  } break;
    case Key__shift:   { mod = OS_Event_modifier__shift;   } break;  
    case Key__control: { mod = OS_Event_modifier__control; } break;
  }
  return mod;
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

Key key_from_os_event_mod(OS_Event_modifier mod)
{
  Key key = Key__NONE;
  switch (mod)
  {
    default: { InvalidCodePath(); } break;
    case OS_Event_modifier__NONE:    { key = Key__NONE;    } break;
    case OS_Event_modifier__shift:   { key = Key__shift;   } break;
    case OS_Event_modifier__control: { key = Key__control; } break;
  }
  return key;
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

  B32 is_event_made = false;
  OS_Event event = {};

  LRESULT result = {};
  switch (message)
  {
    default: { result = DefWindowProc(window_handle, message, w_param, l_param); } break;
    
    case WM_SYSKEYDOWN: 
    case WM_SYSKEYUP: 
    {
      // note: Just dont have a clear thing i need for this yet, but i know that i might need this at some point, 
      //       so leaving this in here with a BP to know when i need this.
      BP; 
      result = DefWindowProc(window_handle, message, w_param, l_param);
    } break;

    case WM_KEYDOWN: 
    case WM_KEYUP: 
    {
      Key key = {};

      // Getting the key
      if (0) {}
      else if ('A' <= w_param && w_param <= 'Z') { key = (Key)((U32)Key__a + (w_param - 'A')); }
      else if ('0' <= w_param && w_param <= '9') { key = (Key)((U32)Key__0 + (w_param - '0')); }
      else {
        switch (w_param)
        {
          default:         { /*InvalidCodePath();*/  } break;
          case VK_SHIFT:   { key = Key__shift;       } break;
          case VK_CONTROL: { key = Key__control;     } break;
          case VK_DELETE:  { key = Key__delete;      } break;
          case VK_TAB:     { key = Key__tab;         } break;
          case VK_LEFT:    { key = Key__left_arrow;  } break;
          case VK_UP:      { key = Key__up_arrow;    } break;
          case VK_RIGHT:   { key = Key__right_arrow; } break;
          case VK_DOWN:    { key = Key__down_arrow;  } break;
          case VK_SPACE:   { key = Key__space;       } break;
        }
      }

      // Unwrapping the message data
      B32 went_down   = false;
      B32 got_up      = false;
      B32 repeat_down = false;
      {
        U16 key_repeat_count = (U16)l_param; 
        U8 scan_code         = (U8)(l_param >> 16);
        B32 is_extended_key  = !!(l_param & bit_24);
        B32 context_code     = !!(l_param & bit_29);
        B32 prev_key_state   = !!(l_param & bit_30);
        B32 transition_state = !!(l_param & bit_31); // Always 0 for WM_KEYDOWN

        got_up      = (transition_state != 0);
        went_down   = (message == WM_KEYDOWN && prev_key_state == 0); 
        repeat_down = (message == WM_KEYDOWN && prev_key_state == 1); 
        
        // Just making sure
        if (!repeat_down) { Assert(XOR(went_down, got_up)); }
        if (!got_up) { Assert(XOR(went_down, repeat_down)); }
      }

      // VK_SHIFT, VK_CONTROL, and VK_MENU
      OS_Event_modifiers modifiers = {}; 
      if (GetKeyState(VK_SHIFT) & 0x8000) { modifiers |= OS_Event_modifier__shift; }
      if (GetKeyState(VK_CONTROL) & 0x8000) { modifiers |= OS_Event_modifier__control; }

      if (key != Key__NONE)
      {
        is_event_made = true;
        event.kind = OS_Event_kind__key;
        event.key_event.modifiers   = modifiers;
        event.key_event.key         = key;
        event.key_event.went_down   = went_down;
        event.key_event.went_up     = got_up;
        event.key_event.repeat_down = repeat_down;

        is_event_made = true;
      }
    } break;

    // todo: look into WM_CHAR (https://learn.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input)

    case WM_LBUTTONDOWN: case WM_LBUTTONUP: 
    case WM_RBUTTONDOWN: case WM_RBUTTONUP:
    case WM_MBUTTONDOWN: case WM_MBUTTONUP:
    case WM_XBUTTONDOWN: case WM_XBUTTONUP:
    {
      OS_Event_modifiers modifiers = {};
      Mouse_button button          = {};
      B32 went_down                = {};
      B32 got_up                   = {};
      V2F32 mouse_pos              = {};

      U64 win32_event_modifiers = w_param;
      if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP) { button = Mouse_button__left; went_down = (message == WM_LBUTTONDOWN); got_up = (message == WM_LBUTTONUP); }
      if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) { button = Mouse_button__right; went_down = (message == WM_RBUTTONDOWN); got_up = (message == WM_RBUTTONUP); }
      if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP) { button = Mouse_button__middle; went_down = (message == WM_MBUTTONDOWN); got_up = (message == WM_MBUTTONUP); }
      if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP) 
      { 
        win32_event_modifiers = (U64)GET_KEYSTATE_WPARAM(w_param); 
        if (GET_XBUTTON_WPARAM(w_param) == XBUTTON1) { button = Mouse_button__side_near; }
        if (GET_XBUTTON_WPARAM(w_param) == XBUTTON2) { button = Mouse_button__side_far; }
        went_down = (message == WM_XBUTTONDOWN); 
        got_up = (message == WM_XBUTTONUP); 
      }

      // note: We are making the mouse positions relative to windows that they are for, 
      //       so relative to the client areas. l_param is relative to a client area already.
      mouse_pos = v2f32((F32)GET_X_LPARAM(l_param), (F32)GET_Y_LPARAM(l_param));
      
      if (win32_event_modifiers & MK_SHIFT) { modifiers |= OS_Event_modifier__shift; }            
      if (win32_event_modifiers & MK_CONTROL) { modifiers |= OS_Event_modifier__control; }          

      event.kind      = OS_Event_kind__mouse;
      event.mouse_event.modifiers = modifiers;
      event.mouse_event.button    = button;
      event.mouse_event.went_down = went_down;
      event.mouse_event.went_up    = got_up;
      event.mouse_event.mouse_pos = mouse_pos;

      is_event_made = true;
      result = TRUE;
    } break;

    case WM_MOUSEWHEEL:
    {
      // note: Not using any other data that is provided by the event since i dont know if i need it yet
      U32 other_key_states = (U32)GET_KEYSTATE_WPARAM(w_param);
      F32 wheel_delta = (F32)(GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA);
      V2F32 mous_pos = v2f32((F32)GET_X_LPARAM(l_param), (F32)GET_Y_LPARAM(l_param));
      
      OS_Event_modifiers modifiers = {};

      if (GetKeyState(VK_SHIFT) & 0x8000) { modifiers |= OS_Event_modifier__shift; }
      if (GetKeyState(VK_CONTROL) & 0x8000) { modifiers |= OS_Event_modifier__control; }

      event.kind                    = OS_Event_kind__wheel;
      event.wheel_event.modifiers   = modifiers;
      event.wheel_event.scroll_data = wheel_delta;

      is_event_made = true;
    }

    case WM_CAPTURECHANGED:
    {
      // note: This is called to use as a callback to let us know that some other window
      //       is now capturing the mouse and we lost the capture.
      //       There is no message for a callback when we start capturing.
    } break;

    case WM_NCCALCSIZE:
    {
      #ifdef MAIN_IS_DEBUGGING
      if (MAIN_IS_DEBUGGING) {
        result = DefWindowProc(window_handle, message, w_param, l_param);
      } 
      #endif

      // result = 0;
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

  if (is_event_made)
  {
    OS_Event* new_event = ArenaPush(win32_state->frame_arena, OS_Event);
    *new_event = event;
    DllPushBack(&win32_state->frame_event_list, new_event);
    win32_state->frame_event_list.count += 1;
  } 

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

void os_set_cursor(OS_Cursor cursor)
{
  StaticAssert(OS_Cursor__COUNT == OS_Cursor__crosshair + 1); // Making sure thet i dont forget to add code to this func if i add a new cursor

  HCURSOR cursor_handle = {};
  switch (cursor) 
  {
    default:                   { cursor_handle = LoadCursor(Null, IDC_ARROW); } break;
    case OS_Cursor__arrow:     { cursor_handle = LoadCursor(Null, IDC_ARROW); } break;
    case OS_Cursor__hand:      { cursor_handle = LoadCursor(Null, IDC_HAND);  } break;
    case OS_Cursor__crosshair: { cursor_handle = LoadCursor(Null, IDC_CROSS); } break;
  }
  SetCursor(cursor_handle);
}

OS_Cursor os_get_cursor()
{
  StaticAssert(OS_Cursor__COUNT == OS_Cursor__crosshair + 1); // Making sure thet i dont forget to add code to this func if i add a new cursor

  HCURSOR cursor_handle = GetCursor();
  Handle(cursor_handle != Null);

  HCURSOR win32_arrow = LoadCursor(Null, IDC_ARROW);
  HCURSOR win32_hand  = LoadCursor(Null, IDC_HAND);
  HCURSOR win32_cross = LoadCursor(Null, IDC_CROSS);

  OS_Cursor cursor = OS_Cursor__arrow;
  if      (cursor_handle == win32_arrow) { cursor = OS_Cursor__arrow; }
  else if (cursor_handle == win32_hand)  { cursor = OS_Cursor__hand; }
  else if (cursor_handle == win32_cross) { cursor = OS_Cursor__crosshair; }
  return cursor;
}

void os_show_cursor(B32 show)
{
  ShowCursor(show);
}

#endif