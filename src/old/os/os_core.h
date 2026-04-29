#ifndef OS_OS_H
#define OS_OS_H

#include "core/core_include.h"

enum OS_File_access : U32 {
  OS_File_access__share_read     = (1 << 0),
  OS_File_access__share_write    = (1 << 1),
  OS_File_access__read           = (1 << 2),
  OS_File_access__write          = (1 << 3),
  OS_File_access__append         = (1 << 4),

  OS_File_access__visible_read   = OS_File_access__read|OS_File_access__share_read,
  OS_File_access__visible_write  = OS_File_access__write|OS_File_access__share_read,
  OS_File_access__visible_append = OS_File_access__append|OS_File_access__share_read,
};  
typedef U32 OS_File_access_flags; 

struct OS_File      { U64 u64; };
struct OS_Dir       { U64 u64; };
struct OS_Thread    { U64 u64; };
struct OS_Mutex     { U64 u64; };
struct OS_Cond_var  { U64 u64; };
struct OS_Semaphore { U64 u64; };
struct OS_Handle    { U64 u64; }; // This is for stuff that i dont feel like a need a separate type

enum OS_Error : U32 {
  OS_Error__NONE,
  OS_Error__no_such_path,
  OS_Error__access_denied,
  OS_Error__already_exists,
};  

struct OS_File_properties {
  U64 creation_time;
  U64 last_access_time;
  U64 last_write_time;
  U64 size;
  B8 is_directory;
};

// -  init/release
void os_init();
void os_release();

// - file handles
B32 os_file_handle_match(OS_File handle, OS_File other);
B32 os_file_is_valid(OS_File file);
// OS_File os_file_handle_zero();

// - dir handles
B32 os_dir_handle_match(OS_Dir handle, OS_Dir other);
B32 os_dir_is_valid(OS_Dir file);
// OS_Dir os_dir_handle_zero();

// - other handles
B32 os_handle_match(OS_Handle handle, OS_Handle other);
B32 os_handle_is_valid(OS_Handle handle);
// OS_Handle os_handle_zero();

// - file open/close
OS_File os_file_open_ex(Str8 file_name, OS_File_access_flags acess_flags, OS_Error* out_error);
OS_File os_file_open(Str8 file_name, OS_File_access_flags acess_flags);
void os_file_close(OS_File* file);
#define OS_FileOpenClose(file_var_name, file_path, access_flags) DeferInitReleaseLoop(OS_File file_var_name = os_file_open(file_path, access_flags), os_file_close(&file_var_name))

// - file manipulations (OS specific)
U64 os_file_read(OS_File file, Data_buffer* out_buffer);
U64 os_file_write_range(OS_File file, Data_buffer buffer, U64 range_start, U64 range_end);
OS_Error os_file_delete(Str8 file_path);
void os_file_clear(OS_File file);

// TODO: This is not a good way to do this, i cant think of a better way.
void os_win32_set_thread_context(Thread_context* context);
// WE have thread context, but since we link the win23 code it has its own uninitialised thread context

// - file manipulations (OS non specific)
// note: Might have a func like write_buffer and it will return how many bites were written 
//       and then a quick write func that will return if the string/buffer got fully written 
U64 os_file_write(OS_File file, Data_buffer buffer);
void os_file_clear_at_path(Str8 path);
void os_file_copy(Str8 original_file_path, Str8 new_file_path);

// - file data
U64 os_file_get_size(OS_File file);
OS_File_properties os_file_get_properties(OS_File file);

// - directories open/close
OS_Dir os_dir_open(Str8 dir_path);
void os_dir_close(OS_Dir* dir);
OS_Error os_dir_create(Str8 dir_path);
#define OS_DirOpenClose(dir_var_name, dir_path) DeferInitReleaseLoop(OS_Dir dir_var_name = os_dir_open(dir_path), os_dir_close(&dir_var_name))

// - time
Readable_time os_get_current_readable_time();
Time os_get_current_time();
U64 os_get_perf_counter();
U64 os_get_perf_freq_per_sec();
U64 os_get_mouse_double_click_max_time_ms();
U64 os_get_keyboard_initial_repeat_delay();
U64 os_get_keyboard_subsequent_repeat_delay();

// - dlls
OS_Handle os_load_dll(Str8 path_to_dll);
void os_unload_dll(OS_Handle* dll_handle);
void* os_load_proc_from_dll(OS_Handle dll_handle, Str8 proc_name);

// todo: Look into this --> GetFinalPathNameByHandleW

// - threads
typedef void OS_Thread_proc_ft (void* user_data);
OS_Thread os_thread_alloc(OS_Thread_proc_ft* proc_func, void* caller_data);
void os_thread_release(OS_Thread* handle);
void os_thread_join(OS_Thread handle);

// - mutexes
OS_Mutex os_mutex_alloc();
void os_mutex_release(OS_Mutex* handle);
void os_mutex_lock(OS_Mutex handle);
void os_mutex_unlock(OS_Mutex handle);
#define OS_MutexLockScope(handle) DeferInitReleaseLoop(os_mutex_lock(handle), os_mutex_unlock(handle)) 

// - conditional variables
OS_Cond_var os_cond_var_alloc();
void os_cond_var_release(OS_Cond_var* handle);
void os_cond_var_wait(OS_Cond_var cond_var_handle, OS_Mutex mutex_handle);
void os_cond_var_signal(OS_Cond_var cond_var_handle);
void os_cond_var_signal_all(OS_Cond_var cond_var_handle);

// - semaphores
OS_Semaphore os_semaphore_alloc(U16 intial_count, U16 max_count);
void os_semaphore_release(OS_Semaphore* sem);
void os_semaphore_wait(OS_Semaphore sem);
void os_semaphore_signal(OS_Semaphore sem);

// - other
void os_sleep(U64 milliseconds);
U32 os_get_number_of_threads();


struct OS_Image { // For now the convention is PX (U8 r, U8 P, U8 b, U8 a);
  U8* data;
  U64 width;  // px
  U64 height; // px
};
OS_Image os_take_screenshot(Arena* arena);
void make_window_not_capture_events();

// - todo:
Str8_list os_test_list_directory(Arena* arena, Str8 dir_path);

///////////////////////////////////////////////////////////
// Damian: Process info
//
struct OS_Process_info {
  U64 pid;
  U64 ppid;
  Readable_time creation_time;
  Readable_time exit_time;
  Readable_time kernel_time;
  Readable_time user_time;
  B8 has_window;
  Str8 full_path;
};
struct OS_Process_info_node {
  OS_Process_info info;
  OS_Process_info_node* next;
};
struct OS_Process_info_list {
  OS_Process_info_node* first;
  U64 count;
};
OS_Process_info_list os_get_info_for_running_processes(Arena* arena);

///////////////////////////////////////////////////////////
// Damian: Process icons
//
struct OS_Process_icon {  
  U64 width;
  U64 height;
  Data_buffer buffer;
};
struct OS_Process_icon_node {
  OS_Process_icon data;
  OS_Process_icon_node* next;
};
struct OS_Process_icon_list {
  OS_Process_icon_node* first;
  U64 count;
};
OS_Process_icon_list os_get_icon_list_for_process(Arena* arena, Str8 full_path);




// ---- testing some stuff
B32 find_window_with_name_and_return_true_if_exists_and_move_that_window_to_the_foreground(Str8 window_name);






#endif


