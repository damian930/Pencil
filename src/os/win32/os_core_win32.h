#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

#include "os/os_core.h" // todo: I am not sure if this is supposed to include this

// todo: Check if these are better here or in the cpp file 
#pragma comment(lib, "version.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")  

#include "windows.h"
#include "oleacc.h"
#include "tlhelp32.h"
#include "shellapi.h"

// note: Some of the os resources that either have to be allocated or opened are stored inside the os layer memory.
//       Not all the stuff to which a handle might be open is stored there. Things like files and dlls are not.
//       Some things are. For example threads. They require additional data stored along them and dont merely wrap the 
//       underlying OS handle (eg: HANDLE). For this reason a dynamic storage is needed, hence the os layer memeory for threads.

// note: Threads need to be dynamically allocated since they store additinal data other then win32 HANDLE.
//       That additional data is the caller_data and the proc function
struct OS_Win32_thread {
  HANDLE win32_handle;
  void* caller_data;
  Str8 name; // todo: Where is this used ???
  void (*proc_func) (void* caller_data);

  // Free list stuff, managed by the allocation functions
  OS_Win32_thread* next_free;
  B8 is_active;                // note: this might be a bad name. Its not about active like a thread is still running, its about it not beeing a zero thread inside the os pool buffer for threads
};

// note: Mutexes need to be dynamically allocated since they are not returned by a handle from win32.
//       They are returned by a struct CRITICAL_SECTION. It contains different handles,
//       but itself is a local space value. Since we need it to be references outside the os layer win32 specific
//       implementation, it has to be allocated in the layer for us to then be able to reference it with an opaque handle.
struct OS_Win32_mutex {
  CRITICAL_SECTION critical_section;
  
  // Free list stuff, managed by the allocation functions
  OS_Win32_mutex* next_free;
  B8 is_active;  
};

// note: Mutexes need to be dynamically allocated since they are not returned by a handle from win32.
//       They are returned by a struct CONDITION_VARIABLE. It is a local space value. 
//       Since we need it to be references outside the os layer win32 specific
//       implementation, it has to be allocated in the layer for us to then be able to reference it with an opaque handle.
struct OS_Win32_cond_var {
  CONDITION_VARIABLE cond_var;
  
  // Free list stuff, managed by the allocation functions
  OS_Win32_cond_var* next_free;
  B8 is_active;  
};

// note: OS state has to be locked when allocating resources as well, 
//       since we might expect this to be used from differen threads as well.
//       A thread migth want to create other threads while main thread is creating some. 
//       --> Race condition. 
struct OS_Win32_state {
  B32 is_initialised;
  U64 perf_freq_count_per_sec;

  CRITICAL_SECTION state_resource_lock;

  // Memory for threads
  Arena* threads_arena;
  OS_Win32_thread* threads_arr_base;
  OS_Win32_thread* first_free_thread;
  U64 threads_allocated_count;
  U64 next_thread_id;

  // Memory for mutexes
  Arena* mutexes_arena;
  OS_Win32_mutex* mutexes_arr_base;
  OS_Win32_mutex* first_free_mutex;
  U64 mutexes_allocated_count;

  // Memory for cond vars
  Arena* cond_vars_arena;
  OS_Win32_cond_var* cond_vars_arr_base;
  OS_Win32_cond_var* first_free_cond_var;
  U64 cond_vars_allocated_count;
};

extern OS_Win32_state os_win32_g_state;

// - thread safe thread resource allocations
OS_Win32_thread* os_win32_alloc_thread_os_layer_mem_thread_safe();
void os_win32_release_thread_os_layer_mem_thread_safe(OS_Win32_thread* thread);

// - thread safe mutex resource allocations
OS_Win32_mutex* os_win32_alloc_mutex_os_layer_mem__thread_safe();
void os_win32_release_mutex_os_layer_mem__thread_safe(OS_Win32_mutex* mutex);

// - thread safe cond var resource allocations
OS_Win32_cond_var* os_win32_alloc_cond_var__thread_safe();
void os_win32_release_cond_var__thread_safe(OS_Win32_cond_var* cond_var);

// - threads
DWORD os_win32_thread_proc(void* caller_data);

// FileTimeToSystemTime
// 0x8000000000000000
Readable_time os_win32_readable_time_from_systemtime(SYSTEMTIME* sys_time);
Readable_time os_win32_readable_time_from_filetime(FILETIME* filetime);

static inline BOOL os_win32_EnumWindowProc(HWND window_h, LPARAM pid)
{
  DWORD process_id_that_created_the_window = 0;
  LONG_PTR attrs = GetWindowLongPtr(window_h, GWL_STYLE);
  if (   attrs 
      // && (attrs & WS_VISIBLE) 
      && !(attrs & WS_POPUP)
      && !(attrs & WS_MINIMIZE)
      && !(attrs & WS_MINIMIZEBOX)
      && GetWindowThreadProcessId(window_h, &process_id_that_created_the_window)
  ) {
    if ((U64)pid == (U64)process_id_that_created_the_window)
    {
      SetLastError(1 << 29);
      return false;
    }

  } 
  return true;
}

static inline B8 os_win32_does_process_have_a_window(U64 pid)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setlasterror (the Remarks section)
  // Error codes are 32-bit values (bit 31 is the most significant bit). 
  // Bit 29 is reserved for application-defined error codes; 
  // no system error code has this bit set. 
  // If you are defining an error code for your application, 
  // set this bit to indicate that the error code has been defined by 
  // your application and to ensure that your error code does not conflict 
  // with any system-defined error codes.
  B8 does_have_a_window = false;
  BOOL succ = EnumWindows(os_win32_EnumWindowProc, pid);
  if (!succ && GetLastError() == (1 << 29))
  {
    does_have_a_window = true;
  }
  return does_have_a_window;
}




#endif
