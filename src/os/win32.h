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
  Key__Shift,
  Key__Control,

  // Letters 
  Key__A,              // 'a'   shifted: 'A'
  Key__B,              // 'b'   shifted: 'B'
  Key__C,              // 'c'   shifted: 'C'
  Key__D,              // 'd'   shifted: 'D'
  Key__E,              // 'e'   shifted: 'E'
  Key__F,              // 'f'   shifted: 'F'
  Key__G,              // 'P'   shifted: 'P'
  Key__H,              // 'h'   shifted: 'H'
  Key__I,              // 'i'   shifted: 'I'
  Key__J,              // 'j'   shifted: 'J'
  Key__K,              // 'k'   shifted: 'K'
  Key__L,              // 'l'   shifted: 'L'
  Key__M,              // 'm'   shifted: 'M'
  Key__N,              // 'n'   shifted: 'N'
  Key__O,              // 'o'   shifted: 'O'
  Key__P,              // 'p'   shifted: 'P'
  Key__Q,              // 'q'   shifted: 'Q'
  Key__R,              // 'r'   shifted: 'R'
  Key__S,              // 's'   shifted: 'S'
  Key__T,              // 't'   shifted: 'T'
  Key__U,              // 'u'   shifted: 'U'
  Key__V,              // 'v'   shifted: 'V'
  Key__W,              // 'w'   shifted: 'W'
  Key__X,              // 'x'   shifted: 'X'
  Key__Y,              // 'y'   shifted: 'Y'
  Key__Z,              // 'z'   shifted: 'Z'

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
  Key__Space,          // ' '   shifted: ' '
  Key__Backtick,       // '`'   shifted: '~'
  Key__Minus,          // '-'   shifted: '_'
  Key__Equals,         // '='   shifted: '+'
  Key__Left_bracket,   // '['   shifted: '{'
  Key__Right_bracket,  // ']'   shifted: '}'
  Key__Backslash,      // '\'   shifted: '|'
  Key__Semicolon,      // ';'   shifted: ':'
  Key__Apostrophe,     // '\''  shifted: '"'
  Key__Comma,          // ','   shifted: '<'
  Key__Period,         // '.'   shifted: '>'
  Key__Slash,          // '/'   shifted: '?'

  // Other 
  Key__Left_arrow,
  Key__Right_arrow,
  Key__Up_arrow,
  Key__Down_arrow,
  Key__Home,
  Key__End,
  Key__Page_up,
  Key__Page_down,
  Key__Backspace,
  Key__Delete,
  Key__Insert,
  Key__Escape,
  Key__Tab,
  Key__Enter,
  Key__Caps_lock,

  Key__COUNT,
};

enum Mouse_button : U32 {
  Mouse_button__NONE,
  Mouse_button__Left,
  Mouse_button__Right,
  Mouse_button__Middle,
  Mouse_button__Side_far,
  Mouse_button__Side_near,
  Mouse_button__COUNT,
};

enum OS_Event_modifier : U32 {
  OS_Event_modifier__NONE,
  OS_Event_modifier__Shift,
  OS_Event_modifier__Control,
};
typedef U32 OS_Event_modifiers;

enum OS_Event_kind {
  OS_Event_kind__NONE,
  OS_Event_kind__Key,
  OS_Event_kind__Mouse_went_up,
  OS_Event_kind__Mouse_went_down,
};

struct OS_Key_state {
  Key key;

  B8 is_up;
  B8 is_down;

  B8 was_up;
  B8 was_down;

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
struct OS_File_props {
  B32 succ; // todo: I would like a better name for this
  U64 size;
};
OS_File os_file_handle_zero();
B32 os_file_handle_match(OS_File handle, OS_File other);
B32 os_file_is_valid(OS_File file);
OS_File_props os_file_get_props(OS_File file);
OS_File os_file_open_ex(Str8 file_name, OS_File_access_flags acess_flags, OS_Error* out_error);
OS_File os_file_open(Str8 file_name, OS_File_access_flags acess_flags);
void os_file_close(OS_File* file);
// U64 os_file_read(OS_File file, Data_buffer* buffer);
#define OS_FileOpenClose(file_var_name, file_path, access_flags) DeferInitReleaseLoop(OS_File file_var_name = os_file_open(file_path, access_flags), os_file_close(&file_var_name))

void os_get_current_dir_path();

// - Frame
void os_frame_begin();
void os_frame_end();

// - Windowing
V2F32 os_get_window_dims();
V2F32 os_get_client_area_dims();
V2F32 os_get_mouse_pos();         // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_prev_mouse_pos();    // note: This is relative to the signle window's client area we have right now in the app
V2F32 os_get_mouse_delta();
B32 os_window_should_close();
void os_window_maximize();
void os_window_minimize();
// void os_window_set_top_most(B32 b);

// - Inputs for keyboard
OS_Key_state os_get_key_state(Key key);
B32 os_key_down(Key key);
B32 os_key_up(Key key);
B32 os_key_went_down(Key key);
B32 os_key_went_up(Key key);
Str8 str8_from_os_key(Key key);

// - Inputs for mouse
OS_Mouse_button_state os_get_mouse_button_state(Mouse_button button);
B32 os_mouse_button_down(Mouse_button button);
B32 os_mouse_button_up(Mouse_button button);
B32 os_mouse_button_went_down(Mouse_button button);
B32 os_mouse_button_went_up(Mouse_button button);

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
Str8 os_get_path_to_system_fonts();
Str8 str8_from_wstr(Arena* arena, WCHAR* wstr);



#endif