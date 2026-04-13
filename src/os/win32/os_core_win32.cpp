#ifndef OS_CORE_WIN32_CPP
#define OS_CORE_WIN32_CPP

#include "os/os_core.h"
#include "os/win32/os_core_win32.h" 

#include "core/core_include.cpp"

// note: I kinda battled for a while with what flags i want to have to open files.
//       I dont know if this is a thing on other platforms, but it is on windows,
//       there is a file opening flag 'CREATE_NEW' which only create if a file does not exist,
//       it is supposed to be synchronised on the os leverl.
//       To support that, i would have to have my flags be the following:
//       - share_read
//       - share_write
//       - open
//       - create
//       - truncate
//       These can cover thole wheole creation disposition list for win32 for files.
//       CREATE_ALWAYS :: open + create + truncate
//       CREATE_NEW :: create
//       OPEN_ALWAYS :: open + create
//       OPEN_EXISTING :: open
//       TRUNCATE_EXISTING :: open + truncate
//       i dont know if i like this, cause its too verbose maybe, will see.
//       if i need those, i will just make a os_file_open_ex or something with Ex flags.
//       For now i am just going to stick to these. I got them from raddbg codebase.
//       Ryan uses a different set in his 'Digital Grove' codebase. 
//       Not that i have to use these, my set was fine also, but i was just more concentrated
//       on being able to represent each win32 flag as a combination of lower level flags.
//       Those do that, but i just dont have enough experience to know if i need all those options.
//       
//       I could have them, but them a couple that just made of the other,
//       like if i wanter the 'write' flags, i would just have it be open + create + truncate --> CREATE_ALWAYS,
//       this would always provide me with a file.
//       
//       And if i wanted to just have it be opened for read i would just use open --> OPEN_EXISTING.
//
//       And if i wanted to have it as append, then it would be open + create --> OPEN_ALWAY 
//
//       Then these extended flags would have their own file open func, 
//       but then for fast path, i would have the 'normal_casr' func that would jsut take the
//       non extended flags and then convert them to the extended flags and call the
//       extended open for the file. 
//
//       I am pretty sure i am overthing this a bit, but i just wanted to be able to 
//       have the complete top level functionality that win32 gives me without
//       sacroficing the api.
//       Having the ability to have Ex and regular file flags is a great idea for these I feel like.
//       
//       Having said that, imma stick to easier once for now, will see if i need the more complicated once.

OS_Win32_state os_win32_g_state = {};

///////////////////////////////////////////////////////////
// - init/release
//
void os_init()
{
  BOOL succ = {};
  
  LARGE_INTEGER freq_lr = {};
  succ = QueryPerformanceFrequency(&freq_lr); Assert(succ);
  
  InitializeCriticalSection(&os_win32_g_state.state_resource_lock);

  os_win32_g_state.is_initialised = true;
  os_win32_g_state.perf_freq_count_per_sec = freq_lr.QuadPart; 

  os_win32_g_state.threads_arena = arena_alloc(Megabytes(64));
  os_win32_g_state.threads_arr_base = ArenaPushArr(os_win32_g_state.threads_arena, OS_Win32_thread, 0);

  os_win32_g_state.mutexes_arena = arena_alloc(Megabytes(64));
  os_win32_g_state.mutexes_arr_base = ArenaPushArr(os_win32_g_state.mutexes_arena, OS_Win32_mutex, 0);

  os_win32_g_state.cond_vars_arena = arena_alloc(Megabytes(64));
  os_win32_g_state.cond_vars_arr_base = ArenaPushArr(os_win32_g_state.cond_vars_arena, OS_Win32_cond_var, 0);
}

void os_release()
{
  DeleteCriticalSection(&os_win32_g_state.state_resource_lock);
  arena_release(os_win32_g_state.threads_arena);
  arena_release(os_win32_g_state.mutexes_arena);
  arena_release(os_win32_g_state.cond_vars_arena);
  os_win32_g_state = {};
}

///////////////////////////////////////////////////////////
// - file handles 
//
B32 os_file_handle_match(OS_File handle, OS_File other)
{
  B32 result = (handle.u64 == other.u64);
  return result;
}

B32 os_file_is_valid(OS_File file)
{
  // B32 result = !os_file_handle_match(file, os_file_handle_zero());
  B32 result = !os_file_handle_match(file, OS_File{});
  return result;
}

///////////////////////////////////////////////////////////
// - dir handles 
//
B32 os_dir_handle_match(OS_Dir handle, OS_Dir other)
{
  B32 result = (handle.u64 == other.u64);
  return result;
}

B32 os_dir_is_valid(OS_Dir file)
{
  B32 result = !os_dir_handle_match(file, OS_Dir{});
  return result;
}

///////////////////////////////////////////////////////////
// - other handles
//
B32 os_handle_match(OS_Handle handle, OS_Handle other)
{
  B32 match = (handle.u64 == other.u64);
  return match;
}

B32 os_handle_is_valid(OS_Handle handle)
{
  B32 is_valid = !os_handle_match(handle, OS_Handle{});
  return is_valid;
}

// OS_Handle os_handle_zero();

///////////////////////////////////////////////////////////
// - file open/close 
//
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
    Null, // More flags and attributes, FILE_FLAG_BACKUP_SEMANTICS has to be used to be able to open a dir
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
  if (file_handle != INVALID_HANDLE_VALUE && file_handle == 0) { InvalidCodePath(); }

  OS_File file = {};
  file.u64 = (file_handle == INVALID_HANDLE_VALUE ? 0 : (U64)file_handle);
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

///////////////////////////////////////////////////////////
// - file manipulations 
//
// TODO: This is not a good way to do this, i cant think of a better way.
void os_win32_set_thread_context(Thread_context* context)
{
  set_thread_context(context);
}
// WE have thread context, but since we link the win23 code it has its own uninitialised thread context

U64 os_file_read(OS_File file, Data_buffer* out_buffer)
{
  if (!os_file_is_valid(file)) { return 0; }

  LARGE_INTEGER new_pointer_pos = {};
  LARGE_INTEGER distance_to_move = {};
  distance_to_move.QuadPart = 0;
  
  BOOL pointer_set_succ = SetFilePointerEx(
    (HANDLE)file.u64, 
    distance_to_move, 
    &new_pointer_pos, 
    FILE_BEGIN
  );
  HandleLater(pointer_set_succ);
  Assert(new_pointer_pos.QuadPart == 0);

  U64 total_count_to_read = out_buffer->count;
  U64 bytes_read = 0;
  for (;;)
  {
    // todo: This will just rewrite the data in the out buffer when we do the second loop, fix this
    U32 bytes_read_this_time = 0;
    BOOL succ = ReadFile(
      (HANDLE)file.u64, 
      (void*)out_buffer->data, 
      (U32)total_count_to_read, 
      (DWORD*)&bytes_read_this_time, 
      Null
    );
    bytes_read += bytes_read_this_time;

    if (bytes_read >= total_count_to_read || bytes_read_this_time == 0) { break; }
    if (!succ)
    {
      // todo: See what up with this
      HandleLater(false);
      break; 
    }
  }

  return bytes_read; 
}

U64 os_file_write_range(OS_File file, Data_buffer buffer, U64 range_start, U64 range_end)
{
  if (!os_file_is_valid(file)) { return 0; }
  if (range_start >= range_end) { return 0;}
  if (range_end > buffer.count) { range_end = buffer.count; }
  
  LARGE_INTEGER new_pointer_pos = {};
  BOOL pointer_set_succ = SetFilePointerEx(
    (HANDLE)file.u64, 
    LARGE_INTEGER{}, 
    &new_pointer_pos, 
    FILE_END
  );
  (void)pointer_set_succ;

  // todo: Make sure this doesnt fail of buffer longer then U32 holds
  U64 bytes_written = 0;
  for (;bytes_written < (range_end - range_start);)
  {
    U32 bytes_written_this_time = 0;
    U64 bytes_to_write = (range_end - range_start) - bytes_written;
    BOOL write_succ = WriteFile((HANDLE)file.u64, buffer.data + range_start + bytes_written, 
                                (U32)bytes_to_write, 
                                (DWORD*)&bytes_written_this_time, Null);
    if (!write_succ) {
      break;
      // todo;
    }
    bytes_written += (U64)bytes_written_this_time;
  }
  return bytes_written;
}

OS_Error os_file_delete(Str8 file_path)
{
  OS_Error result_error = {};
  Scratch scratch = get_scratch(0, 0);
  Str8 file_path_nt = str8_copy_alloc(scratch.arena, file_path);
  BOOL succ = DeleteFileA((char*)file_path_nt.data);
  if (!succ && GetLastError() == ERROR_ACCESS_DENIED) {
    result_error = OS_Error__access_denied;
  } else if (!succ && GetLastError() == ERROR_FILE_NOT_FOUND) {
    result_error = OS_Error__no_such_path;
  }
  end_scratch(&scratch);
  return result_error;
}

void os_file_clear(OS_File file)
{
  if (os_file_handle_match(file, OS_File{})) { return; }
  (void)SetFilePointerEx((HANDLE)file.u64, {}, Null, FILE_BEGIN);
  (void)SetEndOfFile((HANDLE)file.u64);
}

///////////////////////////////////////////////////////////
// - file data 
//
OS_File_properties os_file_get_properties(OS_File file)
{
  if (os_file_handle_match(file, OS_File{})) { return {}; }

  OS_File_properties result_properties = {};
  BY_HANDLE_FILE_INFORMATION file_info = {};
  BOOL succ = GetFileInformationByHandle((HANDLE)file.u64, &file_info);
  HandleLater(succ);
  if (succ)
  {
    FILETIME ft_creation_time    = file_info.ftCreationTime;
    FILETIME ft_last_access_time = file_info.ftLastAccessTime;
    FILETIME ft_last_write_time  = file_info.ftLastWriteTime;

    result_properties.creation_time    = u64_from_2_u32(ft_creation_time.dwHighDateTime, ft_creation_time.dwLowDateTime); 
    result_properties.last_access_time = u64_from_2_u32(ft_last_access_time.dwHighDateTime, ft_last_access_time.dwLowDateTime); 
    result_properties.last_write_time  = u64_from_2_u32(ft_last_write_time.dwHighDateTime, ft_last_write_time.dwLowDateTime); 
    result_properties.size             = u64_from_2_u32(file_info.nFileSizeHigh, file_info.nFileSizeLow);
    result_properties.is_directory     = (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
  return result_properties;
}

///////////////////////////////////////////////////////////
// - directories open/close 
//
OS_Dir os_dir_open(Str8 dir_path)
{
  Scratch scratch = get_scratch(0, 0);
  
  Str8 dir_path_nt = str8_copy_alloc(scratch.arena, dir_path);
  U32 desired_access = 0;
  U32 share_mode = 0;  
  U32 creation_dispostion = OPEN_EXISTING;  

  HANDLE file_handle = CreateFileA(
    (char*)dir_path_nt.data,
    desired_access,
    share_mode,
    Null,
    creation_dispostion,
    FILE_FLAG_BACKUP_SEMANTICS, // More flags and attributes
    Null  // Some template file or something
  );
  
  end_scratch(&scratch);

  // note: The same idea for INVALID_HANDLE_VALUE --> 0 as for file_open
  OS_Dir resutl_dir = {};
  resutl_dir.u64 = (file_handle == INVALID_HANDLE_VALUE ? 0 : (U64)file_handle);
  return resutl_dir;
}

void os_dir_close(OS_Dir* dir)
{
  if (os_dir_handle_match(*dir, OS_Dir{})) { return; }
  BOOL succ = CloseHandle((HANDLE)dir->u64);
  *dir = OS_Dir{};
  (void)succ;
}

OS_Error os_dir_create(Str8 dir_path)
{
  Scratch scratch = get_scratch(0, 0);
  Str8 dir_path_nt = str8_copy_alloc(scratch.arena, dir_path);
  BOOL create_succ = CreateDirectoryA((char*)dir_path_nt.data, Null); // Null is for security attributes
  OS_Error return_error = {};
  if (!create_succ)
  {
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      return_error = OS_Error__already_exists;
    } else if (GetLastError() == ERROR_PATH_NOT_FOUND) {
      return_error = OS_Error__no_such_path;
    }
  }
  end_scratch(&scratch);
  return return_error;
}

///////////////////////////////////////////////////////////
// - time
//
Readable_time os_get_current_readable_time()
{
  SYSTEMTIME sys_time = {};
  GetLocalTime(&sys_time);
  Readable_time time = os_win32_readable_time_from_systemtime(&sys_time);
  return time;
}

Time os_get_current_time()
{
  Readable_time r_time = os_get_current_readable_time();
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
  Assert(os_win32_g_state.is_initialised);
  return os_win32_g_state.perf_freq_count_per_sec;
}

U64 os_get_mouse_double_click_max_time_ms()
{
  U64 time = (U64)GetDoubleClickTime();
  return time;
}

U64 os_get_keyboard_initial_repeat_delay()
{
  U32 value = 0;
  BOOL succ = SystemParametersInfoA(SPI_GETKEYBOARDDELAY, 0, (void*)&value, 0); 
  HandleLater(succ);
  U64 ms = 250 + 250 * value; // note: This formula is based on the win32 docs, link below  
  return ms;
  // link: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfoa
}

U64 os_get_keyboard_subsequent_repeat_delay()
{
  U32 value = 0;
  BOOL succ = SystemParametersInfoA(SPI_GETKEYBOARDSPEED, 0, (void*)&value, 0); 
  HandleLater(succ);
  F32 ratio = ((F32)(value + 1) / (F32)(31 + 1));
  U64 ms = (U64)lerp_f32(2.5f, 30.0f, ratio);
  return ms;
}

///////////////////////////////////////////////////////////
// - dlls
//
OS_Handle os_load_dll(Str8 path_to_dll)
{  
  OS_Handle return_handle = {};
  Scratch scratch = get_scratch(0, 0);
  Str8 dll_name_nt = str8_copy_alloc(scratch.arena, path_to_dll);
  HMODULE module_h = LoadLibraryA((char*)dll_name_nt.data);
  if (module_h)
  {
    return_handle.u64 = (U64)module_h;
  }
  end_scratch(&scratch);
  return return_handle;
}

void os_unload_dll(OS_Handle* dll_handle)
{
  if (!os_handle_is_valid(*dll_handle)) { return; }
  (void)FreeLibrary((HMODULE)dll_handle->u64);
  *dll_handle = OS_Handle{};
}

void* os_load_proc_from_dll(OS_Handle dll_handle, Str8 proc_name)
{
  Scratch scratch = get_scratch(0, 0);
  HMODULE module_h = (HMODULE)dll_handle.u64; // note: HMODULE is already a pointer type
  Str8 proc_name_nt = str8_copy_alloc(scratch.arena, proc_name);
  void* proc = GetProcAddress(module_h, (char*)proc_name_nt.data); // note: This return 0 if failed to load
  end_scratch(&scratch);
  return proc;
}

///////////////////////////////////////////////////////////
// - threads
//
DWORD os_win32_thread_proc(void* caller_data)
{
  // note: Here we are already on another thread in the system
  allocate_thread_context();
  OS_Win32_thread* thread = (OS_Win32_thread*)caller_data;
  thread->proc_func(thread->caller_data);
  release_thread_context();
  return 0;
}

OS_Thread os_thread_alloc(OS_Thread_proc_ft* proc_func, void* caller_data)
{
  OS_Win32_thread* thread = os_win32_alloc_thread_os_layer_mem_thread_safe(); // Have to use the win32 specific allocation since threads use addiditonal info and therefore cant just be wrappers over win32 specific HANDLE.
  thread->caller_data = caller_data;
  thread->proc_func = proc_func;
  thread->win32_handle = CreateThread(Null, 0, os_win32_thread_proc, (void*)thread, CREATE_SUSPENDED, 0);
  DWORD succ = ResumeThread(thread->win32_handle);
  HandleLater(succ != -1);

  OS_Thread handle = { (U64)thread }; 
  return handle; 
}

void os_thread_release(OS_Thread* handle)
{
  if (handle->u64 == 0) { return; }
  OS_Win32_thread* win32_thread = (OS_Win32_thread*)handle->u64;
  BOOL succ = CloseHandle(win32_thread->win32_handle);
  HandleLater(succ);
  os_win32_release_thread_os_layer_mem_thread_safe(win32_thread);
  handle->u64 = 0;
}

void os_thread_join(OS_Thread handle)
{
  if (handle.u64 == 0) { return; }
  OS_Win32_thread* thread = (OS_Win32_thread*)(handle.u64);
  DWORD res = WaitForSingleObject(thread->win32_handle, INFINITE);
  HandleLater(res == WAIT_OBJECT_0);
}

///////////////////////////////////////////////////////////
// - mutexes
//
OS_Mutex os_mutex_alloc()
{
  OS_Win32_mutex* win32_mutex = os_win32_alloc_mutex_os_layer_mem__thread_safe(); // note: this only allocated the dynamic mem for mutex. fill the memory with data here. 
  InitializeCriticalSection(&win32_mutex->critical_section);
  OS_Mutex mutex = {};
  mutex.u64 = (U64)win32_mutex;
  return mutex;
}

void os_mutex_release(OS_Mutex* handle)
{
  if (handle->u64 == 0) { return; }
  OS_Win32_mutex* win32_mutex = (OS_Win32_mutex*)handle->u64;
  DeleteCriticalSection(&win32_mutex->critical_section);
  os_win32_release_mutex_os_layer_mem__thread_safe(win32_mutex);
  handle->u64 = 0;
}

void os_mutex_lock(OS_Mutex handle)
{
  if (handle.u64 == 0) { return; }
  OS_Win32_mutex* mutex = (OS_Win32_mutex*)handle.u64;
  EnterCriticalSection(&mutex->critical_section);
}

void os_mutex_unlock(OS_Mutex handle)
{
  if (handle.u64 == 0) { return; }
  OS_Win32_mutex* mutex = (OS_Win32_mutex*)handle.u64;
  LeaveCriticalSection(&mutex->critical_section);
}

///////////////////////////////////////////////////////////
// - conditional variables 
//
OS_Cond_var os_cond_var_alloc()
{
  OS_Win32_cond_var* cond_var = os_win32_alloc_cond_var__thread_safe();
  cond_var->cond_var = CONDITION_VARIABLE_INIT;
  OS_Cond_var handle = {(U64)cond_var};
  return handle;
}

void os_cond_var_release(OS_Cond_var* handle)
{
  if (handle->u64 == 0) { return; }
  // note: win32 does not require CONDITION_VARIABLE to be released like other handles.
  os_win32_release_cond_var__thread_safe((OS_Win32_cond_var*)handle->u64);
  handle->u64 = 0;
} 

void os_cond_var_wait(OS_Cond_var cond_var_handle, OS_Mutex mutex_handle)
{
  if (cond_var_handle.u64 == 0) { return; }
  if (mutex_handle.u64 == 0)    { return; }

  OS_Win32_cond_var* win32_cond_var = (OS_Win32_cond_var*)cond_var_handle.u64;
  OS_Win32_mutex* mutex = (OS_Win32_mutex*)mutex_handle.u64;

  BOOL succ = SleepConditionVariableCS(&win32_cond_var->cond_var, &mutex->critical_section, INFINITE);
  HandleLater(succ);
}

void os_cond_var_signal(OS_Cond_var cond_var_handle)
{
  OS_Win32_cond_var* win32_cond_var = (OS_Win32_cond_var*)cond_var_handle.u64;
  WakeConditionVariable(&win32_cond_var->cond_var);
}

void os_cond_var_signal_all(OS_Cond_var cond_var_handle)
{
  OS_Win32_cond_var* win32_cond_var = (OS_Win32_cond_var*)cond_var_handle.u64;
  WakeAllConditionVariable(&win32_cond_var->cond_var);
}

///////////////////////////////////////////////////////////
// - semaphores
//
OS_Semaphore os_semaphore_alloc(U16 intial_count, U16 max_count)
{
  Assert(intial_count < max_count);
  Assert(max_count <= LONG_MAX); 

  HANDLE sem_win32_handle = CreateSemaphoreA(0, (LONG)intial_count, (LONG)max_count, 0);  
  HandleLater(sem_win32_handle != Null);

  OS_Semaphore result_sem = {};
  result_sem.u64 = (U64)sem_win32_handle;

  return result_sem;
}

void os_semaphore_release(OS_Semaphore* sem)
{
  if (sem->u64 == 0) { return; }
  CloseHandle((HANDLE)sem->u64);
  sem->u64 = 0;
}

void os_semaphore_wait(OS_Semaphore sem)
{
  if (sem.u64 == 0) { return; }
  HANDLE win32_sem = (HANDLE)sem.u64;
  DWORD ret = WaitForSingleObject(win32_sem, INFINITE);
  HandleLater(ret == WAIT_OBJECT_0);
}

void os_semaphore_signal(OS_Semaphore sem)
{
  // note: ReleaseSemaphore also allows to increase it by more than 1.
  //       Also allows to get the prev semafore value.
  if (sem.u64 == 0) { return; }
  HANDLE win32_sem = (HANDLE)sem.u64;
  BOOL succ = ReleaseSemaphore(win32_sem, 1, Null);
  HandleLater(succ);
}

///////////////////////////////////////////////////////////
// - other
//
void os_sleep(U64 milliseconds)
{
  Assert(milliseconds <= u32_max);
  Sleep((U32)milliseconds);
}

U32 os_get_number_of_threads()
{
  SYSTEM_INFO info = {};
  GetSystemInfo(&info);
  return (U32)info.dwNumberOfProcessors;
}

///////////////////////////////////////////////////////////
// - todo
//
Str8_list os_test_list_directory(Arena* arena, Str8 dir_path)
{
  Str8_list result_list = {};

  HANDLE handle = {};
  WIN32_FIND_DATAA find_data = {};
  {
    Scratch scratch = get_scratch(&arena, 1);
    Str8 dir_path_nt = str8_copy_alloc(scratch.arena, dir_path);
    handle = FindFirstFileA((char*)dir_path.data, &find_data);
    end_scratch(&scratch);
  }
  if (handle != INVALID_HANDLE_VALUE)
  {
    for (;;)
    {
      U8* sub_file_name = (U8*)find_data.cFileName;
      U64 sub_file_name_len = strlen((char*)sub_file_name);

      if (!(sub_file_name_len == 1 && sub_file_name[0] == '.')
          && 
          !(sub_file_name_len == 2 && sub_file_name[0] == '.' && sub_file_name[1] == '.')
      ) { 
        Str8 str_to_store = str8_manuall(sub_file_name, sub_file_name_len);
        str8_list_append_copy(arena, &result_list, str_to_store);
      }

      BOOL succ = FindNextFileA(handle, &find_data);
      if (!succ && GetLastError() == ERROR_NO_MORE_FILES)
      {
        break;
      }
      else if (!succ && GetLastError())
      {
        HandleLater(false);
      } 
    }
  }
  else
  {
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
      // fails because no matching files can be found
    }
  }
  return result_list;
}

///////////////////////////////////////////////////////////
// Damian: Process data 
//
OS_Process_info_list os_get_info_for_running_processes(Arena* arena)
{
  OS_Process_info_list result_list = {};

  HANDLE snapshot_h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot_h == INVALID_HANDLE_VALUE)
  {
    Assert(false);
    // todo: Just return an empty list, but also have something that might specify that creating a snapshot failed for the called above
    //       Maybe just use some sort of string if it is ment for just measseging, or some local codes for this funtion 
  }

  PROCESSENTRY32 pe = {};
  pe.dwSize = sizeof(PROCESSENTRY32); // This for some reason has to be done only for the first process entry in the list
  Process32First(snapshot_h, &pe); // This return BOOL, but doesnt seem like we have to anything about it

  DeferLoop(0, CloseHandle(snapshot_h))
  {
    for (;;)
    {
      HANDLE process_h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
      if (process_h)
      {
        DeferLoop(0, CloseHandle(process_h))
        {
          B32 is_going = true;
          Temp_arena temp_arena = temp_arena_begin(arena);
          
          OS_Process_info_node* new_info_node = ArenaPush(temp_arena.arena, OS_Process_info_node);
    
          new_info_node->info.pid  = pe.th32ProcessID;
          new_info_node->info.ppid = pe.th32ParentProcessID;
    
          /*
          if (is_going)
          { // Getting process name
            if (node->info.full_path.count > 0) // We were able to get the path
            {
              Scratch scratch = get_scratch(arena);
              {
                Data_buffer nt_buffer = data_buffer_make(scratch.arena, node->info.full_path.count);
                ArenaPush(scratch.arena, U8);
                memcpy(nt_buffer.data, node->info.full_path.data, node->info.full_path.count);
                DWORD size_for_version_info = GetFileVersionInfoSize((char*)nt_buffer.data, Null);
                // HandleLater(size_for_version_info != 0);
                if (size_for_version_info)
                {
                  Data_buffer version_info_buffer = data_buffer_make(scratch.arena, size_for_version_info);
                  BOOL succ = GetFileVersionInfoA((char*)nt_buffer.data, Null, version_info_buffer.count, version_info_buffer.data);
                  // HandleLater(succ);
                  if (succ)
                  {
                    struct Win32_lang_and_codepage {
                      WORD wLanguage;
                      WORD wCodePage;
                    } *translation;
                    
                    U32 size_of_data = 0;
                    BOOL succ = VerQueryValue(version_info_buffer.data, 
                                  "\\VarFileInfo\\Translation",
                                  (void**)&translation, // this is void**
                                  &size_of_data);
                    // HandleLater(succ);
    
                    for (U32 i = 0; i < (size_of_data / sizeof(Win32_lang_and_codepage)); i += 1)
                    {
                      Data_buffer buffer = data_buffer_make(scratch.arena, 100);
                      sprintf((char*)buffer.data, 
                        "\\StringFileInfo\\%04x%04x\\FileDescription", 
                        translation[i].wLanguage,
                        translation[i].wCodePage
                      );
    
                      // printf("%s \n", buffer.data);
    
                      // todo: What is this 
                      // Retrieve file description for language and code page "i". 
                      // VerQueryValue(pBlock, 
                      //               SubBlock, 
                      //               &lpBuffer, 
                      //               &dwBytes); 
                    }
    
    
                  }
                }
              }
              end_scratch(&scratch);
            }
          }
          */
    
          if (is_going) // Getting process times
          {
            FILETIME _creation_time = {};
            FILETIME _exit_time     = {};
            FILETIME _kernel_time   = {};
            FILETIME _user_time     = {};
            BOOL succ = GetProcessTimes(process_h, &_creation_time, &_exit_time, &_kernel_time, &_user_time);
            if (succ) 
            {
              Readable_time creation_time = os_win32_readable_time_from_filetime(&_creation_time);
              Readable_time exit_time     = os_win32_readable_time_from_filetime(&_exit_time);
              Readable_time kernel_time   = os_win32_readable_time_from_filetime(&_kernel_time);
              Readable_time user_time     = os_win32_readable_time_from_filetime(&_user_time);
              new_info_node->info.creation_time = creation_time; 
              new_info_node->info.exit_time     = exit_time; 
              new_info_node->info.kernel_time   = kernel_time; 
              new_info_node->info.user_time     = user_time; 
            }
          }
          
          // Getting full path (This shoud be done in the end to make sure 
          //                    that the string is not in the middle of the structure data)
          if (is_going)
          {
            Scratch scratch = get_scratch(&temp_arena.arena, 1);
            Data_buffer image_name_buffer = data_buffer_make(scratch.arena, 256);
            U32 bytes_written = (U32)image_name_buffer.count; 
            BOOL succ = QueryFullProcessImageNameA(process_h, 0, (char*)image_name_buffer.data, 
                                                   (DWORD*)&bytes_written); 
            if (succ) // If GetLastError() == 5 then access to the name of this process was denied
            {
              new_info_node->info.full_path = data_buffer_make(arena, bytes_written);
              memcpy(new_info_node->info.full_path.data, image_name_buffer.data, bytes_written);
            }
            else {
              if (GetLastError() != 5)
              {
                HandleLater(true);
                // BreakPoint();
              }
              is_going = false;
            }
            end_scratch(&scratch);
          }
  
          if (is_going) // Checking if the process has a window
          {
            B8 does_have_a_window = os_win32_does_process_have_a_window(new_info_node->info.pid);  
            new_info_node->info.has_window = does_have_a_window;
          }

          if (is_going)
          {
            StackPush(&result_list, new_info_node);
            result_list.count += 1;
          }
          else 
          {
            temp_arena_end(&temp_arena);
          }
        } // DeferLoop(0, CloseHandle(process_h))
      }
      if (!Process32Next(snapshot_h, &pe)) {
        break;
      }
    } 
  } //DeferLoop(0, CloseHandle(snapshot_h))

  return result_list;
}

OS_Process_icon_list os_get_icon_list_for_process(Arena* arena, Str8 full_path)
{
  OS_Process_icon_list result_list = {};
  Scratch scratch = get_scratch(&arena, 1);
  {
    Str8 full_path_nt = str8_copy_alloc(scratch.arena, full_path);
    ArenaPush(scratch.arena, U8);

    U32 n_icons_in_the_file = ExtractIconExA((char*)full_path_nt.data, -1, Null, Null, 0);

    HICON* small_icons_buffer = ArenaPushArr(scratch.arena, HICON, n_icons_in_the_file); 
    HICON* large_icons_buffer = ArenaPushArr(scratch.arena, HICON, n_icons_in_the_file); 

    U32 n_small_icons_in_the_file = ExtractIconExA((char*)full_path_nt.data, 0, Null, small_icons_buffer, n_icons_in_the_file);
    U32 n_large_icons_in_the_file = ExtractIconExA((char*)full_path_nt.data, 0, large_icons_buffer, Null, n_icons_in_the_file);

    if (n_small_icons_in_the_file != UINT_MAX && n_large_icons_in_the_file != UINT_MAX)
    {
      for EachIndex(small_or_large_icon_index, n_small_icons_in_the_file + n_large_icons_in_the_file)
      {
        HICON icon = {};
        if (small_or_large_icon_index < n_small_icons_in_the_file) 
        {
          icon = small_icons_buffer[small_or_large_icon_index];
        } 
        else if (small_or_large_icon_index < n_small_icons_in_the_file + n_large_icons_in_the_file) 
        {
          icon = small_icons_buffer[small_or_large_icon_index - n_small_icons_in_the_file];
        } 
        else { Assert(false); }

        // todo: Here a condition defer loop would be just great
        ICONINFO icon_info = {}; 
        if (GetIconInfo(icon, &icon_info))
        {
          // todo: There are 2 types of icons, colored and monochrome. I only handle the colored case.
          HBITMAP bitmap_h = icon_info.hbmColor;
          BITMAP bitmap_data = {};
          S32 n_bytes_writen_by_get_object = GetObject(bitmap_h, sizeof(BITMAP), &bitmap_data);
          HandleLater(n_bytes_writen_by_get_object == sizeof(BITMAP));
          
          S32 width = bitmap_data.bmWidth;
          S32 height = bitmap_data.bmHeight;
          S32 n_bits = width * height * 4; // RGBA  
          HandleLater(n_bits != 0); // todo: This has to be handled or just made so this cant happend
          
          OS_Process_icon_node* new_icon_node = ArenaPush(arena, OS_Process_icon_node);
          {
            Data_buffer result_buffer = data_buffer_make(arena, n_bits);  
            
            // note: This copied from claude, the whole getting bytes from win32 is so convoluted,
            //       why isnt there just an example on there.
            BITMAPINFOHEADER bi = {};
            bi.biSize           = sizeof(BITMAPINFOHEADER);
            bi.biWidth          = width;
            bi.biHeight         = -height;  // Top-down
            bi.biPlanes         = 1;
            bi.biBitCount       = 32;
            bi.biCompression = BI_RGB;
            
            HDC hdc = GetDC(Null);
            S32 rows_copied = GetDIBits(hdc, bitmap_h, 0, height, result_buffer.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
            ReleaseDC(Null, hdc); 
            if (rows_copied == width)
            {
              for (U64 i = 0; i < n_bits; i += 4)
              {
                // Win32 stored these as a RGBQUAD structure (wingdi.h), so have to flip them bitches around
                U8 px1 = result_buffer.data[i + 0]; // Blue
                U8 px2 = result_buffer.data[i + 1]; // Green
                U8 px3 = result_buffer.data[i + 2]; // Red
                U8 px4 = result_buffer.data[i + 3]; // Alpha (Reserved)
                (void)px2; (void)px4;
                result_buffer.data[i + 0] = px3; 
                result_buffer.data[i + 2] = px1;
              }
              
              new_icon_node->data.buffer       = result_buffer;
              new_icon_node->data.width        = width;
              new_icon_node->data.height       = height;
              StackPush(&result_list, new_icon_node);
              result_list.count += 1;
            }
            else 
            {
              // todo: Not sure if i like this pop here, would be better to do it less error prone
              arena_pop(arena, result_buffer.count);
            }
          }
        }
        DestroyIcon(icon);
      } // ForEach
    }
  }
  end_scratch(&scratch);
  
  return result_list;
}

// ---------------------------------------------
// -- os_win32 specific stuff

Readable_time os_win32_readable_time_from_systemtime(SYSTEMTIME* sys_time)
{
  Readable_time time = {};
  time.year       = sys_time->wYear;
  time.month      = (Month)(sys_time->wMonth - 1);
  time.day        = (U8)sys_time->wDay;
  time.hour       = (U8)sys_time->wHour;
  time.minute     = (U8)sys_time->wMinute;
  time.second     = (U8)sys_time->wSecond;
  time.milisecond = sys_time->wMilliseconds;
  return time;
}

Readable_time os_win32_readable_time_from_filetime(FILETIME* filetime)
{
  SYSTEMTIME systemtime = {};
  BOOL succ = FileTimeToSystemTime(filetime, &systemtime);
  Assert(succ);
  Readable_time r_time = os_win32_readable_time_from_systemtime(&systemtime);
  return r_time;
}

///////////////////////////////////////////////////////////
// - win32 specific stuff 
//    | This is not the implentation of the abstract api for os stuff.
//    | This is code for win32 specific stuff that then gets used in the
//    | implementation of the abstract os layer stuff.
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// - thread safe thread resource allocations
//
OS_Win32_thread* os_win32_alloc_thread_os_layer_mem_thread_safe()
{
  HandleLater(os_win32_g_state.is_initialised);
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);

  OS_Win32_thread* thread = os_win32_g_state.first_free_thread;
  if (!thread) {
    thread = ArenaPush(os_win32_g_state.threads_arena, OS_Win32_thread);
    os_win32_g_state.threads_allocated_count += 1;
  }
  else {
    StackPop_Name(&os_win32_g_state, first_free_thread, next_free);
  }
  Assert(!thread->is_active);

  *thread = OS_Win32_thread{};
  thread->is_active = true;

  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
  return thread;
}

void os_win32_release_thread_os_layer_mem_thread_safe(OS_Win32_thread* thread)
{
  // todo: There is nothing that prevents the called of this to release the thread twice and therefore add it to the list twice
  HandleLater(os_win32_g_state.is_initialised);
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);
  *thread = OS_Win32_thread{};
  thread->is_active = false;    // Just making the idea behind is_active explicit here
  StackPush_Name(&os_win32_g_state, thread, first_free_thread, next_free);
  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
}

///////////////////////////////////////////////////////////
// - thread safe mutex resource allocations 
//
OS_Win32_mutex* os_win32_alloc_mutex_os_layer_mem__thread_safe()
{
  HandleLater(os_win32_g_state.is_initialised);
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);

  OS_Win32_mutex* mutex = os_win32_g_state.first_free_mutex;
  if (!mutex)
  {
    mutex = ArenaPush(os_win32_g_state.mutexes_arena, OS_Win32_mutex);
    os_win32_g_state.mutexes_allocated_count += 1;
  }
  else 
  {
    StackPop_Name(&os_win32_g_state, first_free_mutex, next_free);
  }
  Assert(!mutex->is_active);

  *mutex = OS_Win32_mutex{};
  mutex->is_active = true;

  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
  return mutex;
}

void os_win32_release_mutex_os_layer_mem__thread_safe(OS_Win32_mutex* mutex)
{
  // todo: There is nothing that prevents the called of this to release the thread twice and therefore add it to the list twice
  HandleLater(os_win32_g_state.is_initialised);
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);
  *mutex = OS_Win32_mutex{};
  mutex->is_active = false;    // Just making the idea behind is_active explicit here
  StackPush_Name(&os_win32_g_state, mutex, first_free_mutex, next_free);
  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
}

///////////////////////////////////////////////////////////
// - thread safe cond var resource allocations
//
OS_Win32_cond_var* os_win32_alloc_cond_var__thread_safe()
{
  HandleLater(os_win32_g_state.is_initialised);
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);

  OS_Win32_cond_var* cond_var = os_win32_g_state.first_free_cond_var;
  if (!cond_var)
  {
    cond_var = ArenaPush(os_win32_g_state.cond_vars_arena, OS_Win32_cond_var);
    os_win32_g_state.cond_vars_allocated_count += 1;
  }
  else 
  {
    StackPop_Name(&os_win32_g_state, first_free_cond_var, next_free);
  }
  Assert(!cond_var->is_active);

  *cond_var = OS_Win32_cond_var{};
  cond_var->is_active = true;

  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
  return cond_var;
}

void os_win32_release_cond_var__thread_safe(OS_Win32_cond_var* cond_var)
{
  // todo: There is nothing that prevents the called of this to release the thread twice and therefore add it to the list twice
  EnterCriticalSection(&os_win32_g_state.state_resource_lock);
  *cond_var = OS_Win32_cond_var{};
  cond_var->is_active = false;    // Just making the idea behind is_active explicit here
  StackPush_Name(&os_win32_g_state, cond_var, first_free_cond_var, next_free);
  LeaveCriticalSection(&os_win32_g_state.state_resource_lock);
}

// -----

B32 find_window_with_name_and_return_true_if_exists_and_move_that_window_to_the_foreground(Str8 window_name)
{
  B32 result = false;
  Scratch scratch = get_scratch(0, 0);
  Str8 name_nt = str8_copy_alloc(scratch.arena, window_name);
  HANDLE mutex_h = CreateMutexA(Null, false, "Pclarity signle test mutex name");
  (void)mutex_h;
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    // get the window of that application and return
    HWND window_h = FindWindowA(Null, (char*)name_nt.data);
    if (window_h)
    {
      SetForegroundWindow(window_h);
      if (IsIconic(window_h))
      {
        ShowWindow(window_h, SW_RESTORE);
      }
      result = true;
    }
  }
  end_scratch(&scratch);
  return result;
}






#endif
