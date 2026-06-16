#ifndef OS_WIN32_H
#define OS_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"
#include "shlobj_core.h"

#include "core/core_include.h"

// Predefines 
struct OS_Window;
struct OS_State;

// Global state for the layer
extern global OS_State* __os_g_state;

///////////////////////////////////////////////////////////
// - Files
//
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

struct OS_File_props {
  B32 succ; // todo: I would like a better name for this
  U64 size;
};

struct OS_File { U64 u64; };

///////////////////////////////////////////////////////////
// - Windowing
//
// ....

///////////////////////////////////////////////////////////
// - Events
// 
enum Key : U32 {
  Key__NONE,

  // Mods
  Key__shift,
  Key__control,
  Key__alt,

  // Letters 
  Key__a,              // 'a'   shifted: 'A'
  Key__b,              // 'b'   shifted: 'B'
  Key__c,              // 'c'   shifted: 'C'
  Key__d,              // 'd'   shifted: 'D'
  Key__e,              // 'e'   shifted: 'E'
  Key__f,              // 'f'   shifted: 'F'
  Key__g,              // 'P'   shifted: 'P'
  Key__h,              // 'h'   shifted: 'H'
  Key__i,              // 'i'   shifted: 'I'
  Key__j,              // 'j'   shifted: 'J'
  Key__k,              // 'k'   shifted: 'K'
  Key__l,              // 'l'   shifted: 'L'
  Key__m,              // 'm'   shifted: 'M'
  Key__n,              // 'n'   shifted: 'N'
  Key__o,              // 'o'   shifted: 'O'
  Key__p,              // 'p'   shifted: 'P'
  Key__q,              // 'q'   shifted: 'Q'
  Key__r,              // 'r'   shifted: 'R'
  Key__s,              // 's'   shifted: 'S'
  Key__t,              // 't'   shifted: 'T'
  Key__u,              // 'u'   shifted: 'U'
  Key__v,              // 'v'   shifted: 'V'
  Key__w,              // 'w'   shifted: 'W'
  Key__x,              // 'x'   shifted: 'X'
  Key__y,              // 'y'   shifted: 'Y'
  Key__z,              // 'z'   shifted: 'Z'

  // Numbers
  Key__0,              // '0'   shifted: ')'
  Key__1,              // '1'   shifted: '!'
  Key__2,              // '2'   shifted: '@'
  Key__3,              // '3'   shifted: '#'
  Key__4,              // '4'   shifted: '$'
  Key__5,              // '5'   shifted: '%'
  Key__6,              // '6'   shifted: '^'
  Key__7,              // '7'   shifted: '&'
  Key__8,              // '8'   shifted: '*'
  Key__9,              // '9'   shifted: '('

  // Other printable 
  Key__space,          // ' '   shifted: ' '
  Key__backtick,       // '`'   shifted: '~'
  Key__minus,          // '-'   shifted: '_'
  Key__equals,         // '='   shifted: '+'
  Key__left_bracket,   // '['   shifted: '{'
  Key__right_bracket,  // ']'   shifted: '}'
  Key__backslash,      // '\'   shifted: '|'
  Key__semicolon,      // ';'   shifted: ':'
  Key__apostrophe,     // '\''  shifted: '"'
  Key__comma,          // ','   shifted: '<'
  Key__period,         // '.'   shifted: '>'
  Key__slash,          // '/'   shifted: '?'

  // Other 
  Key__arrow_left,
  Key__arrow_right,
  Key__arrow_up,
  Key__arrow_down,
  Key__home,
  Key__end,
  Key__page_up,
  Key__page_down,
  Key__backspace,
  Key__delete,
  Key__insert,
  Key__escape,
  Key__tab,
  Key__enter,
  Key__caps_lock,

  Key__COUNT,
};

enum Mouse_button : U32 {
  Mouse_button__NONE,
  Mouse_button__left,
  Mouse_button__right,
  Mouse_button__middle,
  Mouse_button__side_far,
  Mouse_button__side_near,
  Mouse_button__COUNT,
};

// todo: List of events that this shoud support
enum OS_Event_modifier : U32 {
  OS_Event_modifier__NONE    = (1 << 0),
  OS_Event_modifier__shift   = (1 << 1),
  OS_Event_modifier__control = (1 << 2),
  // todo: Add alt here
};
typedef U32 OS_Event_modifiers;

enum OS_Event_kind : U32 {
  OS_Event_kind__NONE,
  OS_Event_kind__mouse,
  OS_Event_kind__key,
  OS_Event_kind__wheel,
};

struct OS_Event {
  OS_Event_kind kind;

  union {
    struct {
      OS_Event_modifiers modifiers;
      Mouse_button button;
      B32 went_down;
      B32 went_up;
      B32 double_down;
      V2F32 mouse_pos;
    } mouse_event;
  
    struct {
      OS_Event_modifiers modifiers;
      Key key;
      B32 went_down;
      B32 went_up;
      B32 repeat_down;
    } key_event;
  
    struct {
      OS_Event_modifiers modifiers;
      F32 scroll_data;
    } wheel_event;
  };

  OS_Event* next;
  OS_Event* prev;
};

struct OS_Event_list {
  OS_Event* first;
  OS_Event* last;
  U64 count;
};

struct OS_Key_state {
  Key key;

  B8 is_up;
  B8 is_down;

  B8 was_up;
  B8 was_down;

  B8 repeat_down;

  B8 is_clicked;
};

struct OS_Mouse_button_state {
  Mouse_button button;
  
  B8 is_up;
  B8 is_down;

  B8 was_up;
  B8 was_down;
};

// - State
void os_init();
void os_release();
OS_State* os_get_state();

// - Files
// todo: Remove this, i dont like this 
// todo: Fix the file api here
enum OS_Error : U32 {
  OS_Error__NONE,
  OS_Error__no_such_path,
  OS_Error__access_denied,
  OS_Error__already_exists,
};
OS_File os_file_handle_zero();
B32 os_file_handle_match(OS_File handle, OS_File other);
B32 os_file_is_valid(OS_File file);
OS_File_props os_file_get_props(OS_File file);
OS_File os_file_open_ex(Str8 file_name, OS_File_access_flags acess_flags, OS_Error* out_error);
OS_File os_file_open(Str8 file_name, OS_File_access_flags acess_flags);
void os_file_close(OS_File* file);
B32 os_file_read(OS_File file, Data_buffer* out_buffer);
B32 os_file_write_end(OS_File file, Data_buffer buffer);
Str8 os_get_current_dir_path(Arena* arena);
#define OS_FileOpenClose(file_var_name, file_path, access_flags) DeferInitReleaseLoop(OS_File file_var_name = os_file_open(file_path, access_flags), os_file_close(&file_var_name))

// - Memory
Mem_chunk os_reserve_mem_chunk(U64 n_pages, B32 start_at_specific_page, U32 allocation_granulatity_index);
B32 os_commit_mem_pages_to_chunk(Mem_chunk* mem_chunk, U64 n_pages);
B32 os_decommit_mem_pages_from_chuck(Mem_chunk* mem_chunk, U64 n_pages);
B32 os_release_mem_chunk(Mem_chunk* mem_chunk);
U64 os_get_mem_page_size();

// note/todo: I dont really like this list api for events, i would rather it just be whatever and the caller just gets them and consumed them based
//            on som api not like we have now. Maybe it would make sense to just have different calls like:
//            os_get_mouse_event() or os_get_keyboard_event() or os_get_system_event() and they are the same struct but we then
//            in those calls specify the event details that we want to get. I dont really like a fat struct approach here that much, or i dont get it.
//            Imma leave it like this for now, cause i am not sure.
void os_frame_begin();
void os_frame_end();
F32 os_get_time_since_last_frame();
OS_Event_list* os_get_frame_event_list();
void os_consume_frame_event(OS_Event* event);

// - Windowing
V2F32 os_get_window_dims();
V2F32 os_get_client_area_dims();
V2F32 os_get_mouse_pos();         // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_prev_mouse_pos();    // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_mouse_delta();
B32 os_window_should_close();
void os_window_maximize();
void os_window_minimize();
B32 os_window_is_transparent();

// Key stuff
Key key_from_str8(Str8 str);
Str8 str8_from_key(Key key);
OS_Event_modifier os_modifier_from_key(Key key);

// - Time
Readable_time os_get_readable_time();
Time os_get_time_ms();
U64 os_get_perf_counter();
U64 os_get_perf_freq_per_sec();
F64 os_get_time_for_timing_sec();
void os_sleep(U64 ms);
// U64 os_get_mouse_double_click_max_time_ms();
// U64 os_get_keyboard_initial_repeat_delay();
// U64 os_get_keyboard_subsequent_repeat_delay();

// - Misc
enum OS_Cursor : U32{
  OS_Cursor__arrow, 
  OS_Cursor__hand,
  OS_Cursor__crosshair,
  OS_Cursor__pen,
  OS_Cursor__COUNT,
};

Str8 os_get_path_to_system_fonts();
Str8 str8_from_wstr(Arena* arena, WCHAR* wstr);
void os_set_cursor(OS_Cursor cursor);
OS_Cursor os_get_cursor();
void os_show_cursor(B32 show);
U64 os_get_mouse_double_click_max_time_ms();

#endif