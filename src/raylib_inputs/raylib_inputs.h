#ifndef RAYLIB_INPUTS_SUCK_MY_DICK_H
#define RAYLIB_INPUTS_SUCK_MY_DICK_H

#include "core/core_include.h"
#include "third_party/raylib/raylib.h"

enum RLI_Key : U32 {
  RLI_Key__NONE,

  // - Modifiers
  RLI_Key__shift,
  RLI_Key__control,

  // - Action keys
  RLI_Key__left_arrow,
  RLI_Key__right_arrow,
  RLI_Key__up_arrow,
  RLI_Key__down_arrow,
  RLI_Key__home,
  RLI_Key__end,
  RLI_Key__page_up,
  RLI_Key__page_down,
  RLI_Key__backspace,
  RLI_Key__delete,
  RLI_Key__insert,
  RLI_Key__escape,
  RLI_Key__tab,
  RLI_Key__enter,
  RLI_Key__caps_lock,

  // idea: Since printable symbols have a variation of shift symbol. Maybe dont have it be mapped manually
  //       or by a routine, but have it be stored here and just be accessed by something like this:
  //       RLI_Key_a + RLI_Shift_modifier. So the modifier will have value of 1 and all the chars that have 
  //       the shift version will just be in order. Like this: a, A, b, B, ...

  // - Printable keys
  RLI_Key__space,         // ' '   shifted: ' '
  RLI_Key__a,             // 'a'   shifted: 'A'
  RLI_Key__b,             // 'b'   shifted: 'B'
  RLI_Key__c,             // 'c'   shifted: 'C'
  RLI_Key__d,             // 'd'   shifted: 'D'
  RLI_Key__e,             // 'e'   shifted: 'E'
  RLI_Key__f,             // 'f'   shifted: 'F'
  RLI_Key__g,             // 'g'   shifted: 'G'
  RLI_Key__h,             // 'h'   shifted: 'H'
  RLI_Key__i,             // 'i'   shifted: 'I'
  RLI_Key__j,             // 'j'   shifted: 'J'
  RLI_Key__k,             // 'k'   shifted: 'K'
  RLI_Key__l,             // 'l'   shifted: 'L'
  RLI_Key__m,             // 'm'   shifted: 'M'
  RLI_Key__n,             // 'n'   shifted: 'N'
  RLI_Key__o,             // 'o'   shifted: 'O'
  RLI_Key__p,             // 'p'   shifted: 'P'
  RLI_Key__q,             // 'q'   shifted: 'Q'
  RLI_Key__r,             // 'r'   shifted: 'R'
  RLI_Key__s,             // 's'   shifted: 'S'
  RLI_Key__t,             // 't'   shifted: 'T'
  RLI_Key__u,             // 'u'   shifted: 'U'
  RLI_Key__v,             // 'v'   shifted: 'V'
  RLI_Key__w,             // 'w'   shifted: 'W'
  RLI_Key__x,             // 'x'   shifted: 'X'
  RLI_Key__y,             // 'y'   shifted: 'Y'
  RLI_Key__z,             // 'z'   shifted: 'Z'
  RLI_Key__0,             // '0'   shifted: ')'
  RLI_Key__1,             // '1'   shifted: '!'
  RLI_Key__2,             // '2'   shifted: '@'
  RLI_Key__3,             // '3'   shifted: '#'
  RLI_Key__4,             // '4'   shifted: '$'
  RLI_Key__5,             // '5'   shifted: '%'
  RLI_Key__6,             // '6'   shifted: '^'
  RLI_Key__7,             // '7'   shifted: '&'
  RLI_Key__8,             // '8'   shifted: '*'
  RLI_Key__9,             // '9'   shifted: '('
  RLI_Key__backtick,      // '`'   shifted: '~'
  RLI_Key__minus,         // '-'   shifted: '_'
  RLI_Key__equals,        // '='   shifted: '+'
  RLI_Key__left_bracket,  // '['   shifted: '{'
  RLI_Key__right_bracket, // ']'   shifted: '}'
  RLI_Key__backslash,     // '\'   shifted: '|'
  RLI_Key__semicolon,     // ';'   shifted: ':'
  RLI_Key__apostrophe,    // '\''  shifted: '"'
  RLI_Key__comma,         // ','   shifted: '<'
  RLI_Key__period,        // '.'   shifted: '>'
  RLI_Key__slash,         // '/'   shifted: '?'

  // - Mouse
  RLI_Key__mouse_left,
  RLI_Key__mouse_right,
  // RLI_Key__mouse_middle,
  // RLI_Key__mouse_side_front,
  // RLI_Key__mouse_side_back,

  RLI_Key__COUNT,
};

enum RLI_Modifier : U32 {
  RLI_Modifier__NONE    = 0,
  RLI_Modifier__shift   = (1 << 0),
  RLI_Modifier__control = (1 << 1),
};
typedef U32 RLI_Modifiers;

struct RLI_Key_state {
  RLI_Key key;

  B8 was_down;
  B8 is_down;

  Time time_in_ms_when_went_down;
  Time last_repeat_time;
  B8 was_initially_repeated; 
};

enum RLI_Event_kind {
  RLI_Event_kind__NONE,
  RLI_Event_kind__keyboard_button,
  RLI_Event_kind__mouse_button,
  RLI_Event_kind__mouse_move,
  RLI_Event_kind__mouse_wheel,
  RLI_Event_kind__window_lost_focus,
  RLI_Event_kind__window_got_fucused,
};

struct RLI_Event {
  RLI_Event_kind kind;
  RLI_Key key;
  Time time_when_occured;

  // todo: Only 1 of these 3 is possible at a time, 
  //       so they shoud be made as enum flags instead
  B8 key_went_down;
  B8 key_came_up;
  B8 key_repeat_down;
  
  RLI_Modifiers modifiers;

  Vec2_F32 mouse_pos;

  F32 mouse_wheel_move;

  RLI_Event* next;
  RLI_Event* prev;
};

struct RLI_Event_list {
  RLI_Event* first;
  RLI_Event* last;
  U64 count;
};

struct RLI_Context {
  Arena* context_arena;
  
  RLI_Key_state key_state_for_prev_frame[RLI_Key__COUNT];
  RLI_Key_state key_state_for_current_frame[RLI_Key__COUNT];

  Time time_in_ms_until_first_keyboard_key_repeat_event;
  Time time_in_ms_between_keyboard_key_repeat_events;

  B8 was_window_focused;

  Arena* frame_event_arena;
  RLI_Event_list frame_event_list;
  Vec2_F32 last_mouse_pos;
};

extern RLI_Context* Raylib_inputs_g_context;

RLI_Context* RLI_get_context();
void RLI_set_context(RLI_Context* context);

void RLI_init(U64 time_in_ms_until_first_keyboard_key_repeat_event, 
              U64 time_in_ms_between_keyboard_key_repeat_events);
void RLI_release();

// todo: This is a bade name, shoud be some like create frame inputs 
RLI_Event_list* RLI_get_frame_inputs();
// void RLI_end_inputs();

RLI_Event_list* RLI_get_event_list();

void RLI_consume_event(RLI_Event* event);

B32 RLI_is_key_down(RLI_Key key);

U8 u8_from_rli_key(RLI_Key key, B32 is_shift_down, B32* out_is_printable);



#endif
