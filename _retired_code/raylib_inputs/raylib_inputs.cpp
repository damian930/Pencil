#ifndef RAYLIB_INPUTS_SUCK_MY_DICK_CPP
#define RAYLIB_INPUTS_SUCK_MY_DICK_CPP

#include "raylib_inputs.h"
#include "core/core_include.cpp"

#include "os/os_core.h"

RLI_Context* Raylib_inputs_g_context = 0;

RLI_Context* RLI_get_context()
{
  return Raylib_inputs_g_context;
}

void RLI_set_context(RLI_Context* context)
{
  Raylib_inputs_g_context = context;
}

void RLI_init(U64 time_in_ms_until_first_keyboard_key_repeat_event, U64 time_in_ms_between_keyboard_key_repeat_events)
{
  Assert(!RLI_get_context());
  
  Arena* arena = arena_alloc(Kilobytes(64));
  RLI_Context* new_context = ArenaPush(arena, RLI_Context);
  RLI_set_context(new_context);
  new_context->context_arena = arena;

  for (U64 i = 0; i < RLI_Key__COUNT; i += 1)
  {
    new_context->key_state_for_prev_frame[i].key = (RLI_Key)i; 
    new_context->key_state_for_current_frame[i].key = (RLI_Key)i;
  }

  new_context->time_in_ms_until_first_keyboard_key_repeat_event = time_in_ms_until_first_keyboard_key_repeat_event;
  new_context->time_in_ms_between_keyboard_key_repeat_events    = time_in_ms_between_keyboard_key_repeat_events;

  new_context->frame_event_arena = arena_alloc(Megabytes(16));
}

void RLI_release()
{
  RLI_Context* context = RLI_get_context();

  context->frame_event_list = {}; // todo: Maybe i shoud just call end frame here, might be nice ???
  arena_release(context->frame_event_arena);

  arena_release(context->context_arena);
  RLI_set_context(0);
}

RLI_Event_list* RLI_get_frame_inputs()
{
  RLI_Context* rli = RLI_get_context();
  arena_clear(rli->frame_event_arena);
  rli->frame_event_list = {};
  
  MemCopySafe(rli->key_state_for_prev_frame, rli->key_state_for_current_frame);

  if ( (!rli->was_window_focused && IsWindowFocused())
    || (rli->was_window_focused && !IsWindowFocused())
  ) {
    RLI_Event* event = ArenaPush(rli->frame_event_arena, RLI_Event);
    event->kind = (IsWindowFocused() ? RLI_Event_kind__window_got_fucused : RLI_Event_kind__window_lost_focus);
    event->time_when_occured = os_get_current_time();
    DllPushBack(&rli->frame_event_list, event);
    rli->frame_event_list.count += 1;
    rli->was_window_focused = IsWindowFocused();
    
    // Reset for key states
    if (!rli->was_window_focused)
    {
      for (U64 i = 0; i < RLI_Key__COUNT; i += 1)
      {
        RLI_Key_state* current_key_state = rli->key_state_for_current_frame + i;
        RLI_Key_state* prev_key_state    = rli->key_state_for_prev_frame + i;
        *current_key_state = {};
        *prev_key_state = {};
        current_key_state->key = (RLI_Key)i;
        prev_key_state->key = (RLI_Key)i;
      }
      return &rli->frame_event_list;
    }
  }

  for (U64 i = 0; i < RLI_Key__COUNT; i += 1)
  {
    RLI_Key_state* current_key_state = rli->key_state_for_current_frame + i;
    RLI_Key_state* prev_key_state    = rli->key_state_for_prev_frame + i;
    RLI_Key rli_key                  = current_key_state->key;

    if (rli_key == RLI_Key__NONE) { continue; }

    B32 do_create_new_event = false;
    B8 was_down             = prev_key_state->is_down;
    B8 is_down              = was_down; 
    B8 is_repeat_down       = false;
    V2F32 mouse_pos        = {}; 
    RLI_Event_kind kind     = {};

    if (RLI_Key__mouse_left <= rli_key && rli_key <= RLI_Key__mouse_right)
    {
      // todo: This mapping here is wrong for non left/right/middle buttons
      MouseButton raylib_mouse_button = (MouseButton)((U32)rli_key - (U32)RLI_Key__mouse_left + (U32)MOUSE_BUTTON_LEFT);
      is_down = IsMouseButtonDown(raylib_mouse_button);
      mouse_pos = v2f32((F32)GetMouseX(), (F32)GetMouseY());
      kind = RLI_Event_kind__mouse_button;

      if (was_down != is_down) {
        do_create_new_event = true;
      }
    }
    else
    {
      KeyboardKey raylib_key = {};
      if      (rli_key == RLI_Key__space)         { raylib_key = KEY_SPACE; }
      else if (rli_key == RLI_Key__shift)         { raylib_key = KEY_LEFT_SHIFT; }    // todo: Not the right one tho
      else if (rli_key == RLI_Key__control)       { raylib_key = KEY_LEFT_CONTROL; }  // todo: Not the right one tho
      else if (rli_key == RLI_Key__left_arrow)    { raylib_key = KEY_LEFT; }
      else if (rli_key == RLI_Key__right_arrow)   { raylib_key = KEY_RIGHT; }
      else if (rli_key == RLI_Key__up_arrow)      { raylib_key = KEY_UP; }
      else if (rli_key == RLI_Key__down_arrow)    { raylib_key = KEY_DOWN; }
      else if (rli_key == RLI_Key__home)          { raylib_key = KEY_HOME; }
      else if (rli_key == RLI_Key__end)           { raylib_key = KEY_END; }
      else if (rli_key == RLI_Key__page_up)       { raylib_key = KEY_PAGE_UP; }
      else if (rli_key == RLI_Key__page_down)     { raylib_key = KEY_PAGE_DOWN; }
      else if (rli_key == RLI_Key__backspace)     { raylib_key = KEY_BACKSPACE; }
      else if (rli_key == RLI_Key__delete)        { raylib_key = KEY_DELETE; }
      else if (rli_key == RLI_Key__insert)        { raylib_key = KEY_INSERT; }
      else if (rli_key == RLI_Key__escape)        { raylib_key = KEY_ESCAPE; }
      else if (rli_key == RLI_Key__tab)           { raylib_key = KEY_TAB; }
      else if (rli_key == RLI_Key__enter)         { raylib_key = KEY_ENTER; }
      else if (rli_key == RLI_Key__caps_lock)     { raylib_key = KEY_CAPS_LOCK; }
      else if (rli_key == RLI_Key__0)             { raylib_key = KEY_ZERO; }
      else if (rli_key == RLI_Key__1)             { raylib_key = KEY_ONE; }
      else if (rli_key == RLI_Key__2)             { raylib_key = KEY_TWO; }
      else if (rli_key == RLI_Key__3)             { raylib_key = KEY_THREE; }
      else if (rli_key == RLI_Key__4)             { raylib_key = KEY_FOUR; }
      else if (rli_key == RLI_Key__5)             { raylib_key = KEY_FIVE; }
      else if (rli_key == RLI_Key__6)             { raylib_key = KEY_SIX; }
      else if (rli_key == RLI_Key__7)             { raylib_key = KEY_SEVEN; }
      else if (rli_key == RLI_Key__8)             { raylib_key = KEY_EIGHT; }
      else if (rli_key == RLI_Key__9)             { raylib_key = KEY_NINE; }
      else if (rli_key == RLI_Key__backtick)      { raylib_key = KEY_GRAVE; }
      else if (rli_key == RLI_Key__minus)         { raylib_key = KEY_MINUS; }
      else if (rli_key == RLI_Key__equals)        { raylib_key = KEY_EQUAL; }
      else if (rli_key == RLI_Key__left_bracket)  { raylib_key = KEY_LEFT_BRACKET; }
      else if (rli_key == RLI_Key__right_bracket) { raylib_key = KEY_RIGHT_BRACKET; }
      else if (rli_key == RLI_Key__backslash)     { raylib_key = KEY_BACKSLASH; }
      else if (rli_key == RLI_Key__semicolon)     { raylib_key = KEY_SEMICOLON; }
      else if (rli_key == RLI_Key__apostrophe)    { raylib_key = KEY_APOSTROPHE; }
      else if (rli_key == RLI_Key__comma)         { raylib_key = KEY_COMMA; }
      else if (rli_key == RLI_Key__period)        { raylib_key = KEY_PERIOD; }
      else if (rli_key == RLI_Key__slash)         { raylib_key = KEY_SLASH; }
      else if (RLI_Key__a <= rli_key && rli_key <= RLI_Key__z) {
        raylib_key = (KeyboardKey)(rli_key - RLI_Key__a + KEY_A);
      }

      // todo: Handle if we dont map a key
      Assert(raylib_key);

      is_down = IsKeyDown(raylib_key);
      kind    = RLI_Event_kind__keyboard_button;

      if (!was_down && is_down)
      {
        current_key_state->time_in_ms_when_went_down = os_get_current_time();
        do_create_new_event = true;
      }
      else if (was_down && is_down)
      {
        Time went_down_ms = current_key_state->time_in_ms_when_went_down;
        Time now_ms = os_get_current_time();
        if ( !current_key_state->was_initially_repeated
          && now_ms - went_down_ms >= rli->time_in_ms_until_first_keyboard_key_repeat_event
        ) {
          is_repeat_down = true;
          current_key_state->was_initially_repeated = true;
          current_key_state->last_repeat_time = now_ms;
          do_create_new_event = true;
        }
        else if (
             current_key_state->was_initially_repeated
          && now_ms - current_key_state->last_repeat_time >= rli->time_in_ms_between_keyboard_key_repeat_events
        ) {
          is_repeat_down = true;
          current_key_state->last_repeat_time = now_ms;
          do_create_new_event = true;
        }
      }
      else if (was_down && !is_down)
      {
        do_create_new_event = true;
        current_key_state->time_in_ms_when_went_down = 0;
        current_key_state->last_repeat_time = 0;
        current_key_state->was_initially_repeated = 0;
      }
    }

    current_key_state->was_down       = was_down;
    current_key_state->is_down        = is_down;

    // Figuring out if we have an event to create 
    if (do_create_new_event)
    {
      RLI_Event* new_event = ArenaPush(rli->frame_event_arena, RLI_Event);
      new_event->kind              = kind;
      new_event->key               = current_key_state->key;
      new_event->time_when_occured = os_get_current_time();
      new_event->key_went_down     = (!was_down && is_down);
      new_event->key_came_up       = (was_down && !is_down);
      new_event->key_repeat_down   = is_repeat_down;
      new_event->mouse_pos         = mouse_pos;

      if (rli->key_state_for_current_frame[RLI_Key__shift].is_down)
      {
        new_event->modifiers |= RLI_Modifier__shift; 
      }
      if (rli->key_state_for_current_frame[RLI_Key__control].is_down)
      {
        new_event->modifiers |= RLI_Modifier__control; 
      }
  
      RLI_Event_list* event_list = &rli->frame_event_list;
      DllPushBack(event_list, new_event);
      event_list->count += 1;
    }
  }

  // note: Since we dont have a list of events from raylib, like we would from the os api like win32,
  //       and we only work with the already abstracted events api to build out own, 
  //       we get a some what worse version of the starting api. 
  //       Some data what would be easy go get from the os is hard to get now,
  //       for example the mouse wheel move event. In win32 it can be seen as a separate event,
  //       but since raylib doesnt expose the event list, but only exposes the final state of
  //       buttons it retains and modifies based on those events, we cant know when the 
  //       mouse wheel ectually was made. 
  //       So we just make it here, in the end.

  F32 wheel_move = GetMouseWheelMove();
  if (wheel_move != 0.0f)
  {
    RLI_Event* wheel_event = ArenaPush(rli->frame_event_arena, RLI_Event);
    wheel_event->time_when_occured = os_get_current_time();
    wheel_event->kind = RLI_Event_kind__mouse_wheel;
    wheel_event->mouse_wheel_move = wheel_move;
    DllPushBack(&rli->frame_event_list, wheel_event);
    rli->frame_event_list.count += 1;
  }

  V2F32 mouse_pos = v2f32((F32)GetMouseX(), (F32)GetMouseY());
  if (!v2f32_match(mouse_pos, rli->last_mouse_pos))
  {
    rli->last_mouse_pos = mouse_pos;
    RLI_Event* mouse_move_event = ArenaPush(rli->frame_event_arena, RLI_Event);
    mouse_move_event->time_when_occured = os_get_current_time();
    mouse_move_event->kind = RLI_Event_kind__mouse_move;
    mouse_move_event->mouse_pos = mouse_pos;
    DllPushBack(&rli->frame_event_list, mouse_move_event);
    rli->frame_event_list.count += 1;
  }
  
  return &rli->frame_event_list;
}

// void RLI_end_inputs()
// {
//   // Nothing here :))
// }

// todo: REmove this, thsi is probably not needed at all, the key access and event acceess api shoud do it
RLI_Event_list* RLI_get_event_list()
{
  RLI_Context* rli = RLI_get_context();
  Assert(rli);
  RLI_Event_list* list = 0;
  if (rli) { list = &rli->frame_event_list; }
  return list;
}

// todo: Maybe also pass in the event list to not look so implicit
//       relative to which list might be affected by the change
void RLI_consume_event(RLI_Event* event)
{
  // todo: Dont know how i feel about this as a list, since there is nothing the prevents
  //       the user from fucking up the event nodes and then there will
  //       be very weird stuff happenning, but for now its fine.
  RLI_Context* context = RLI_get_context();
  DllPop(&context->frame_event_list, event);
  context->frame_event_list.count -= 1;
}

B32 RLI_is_key_down(RLI_Key key)
{
  RLI_Context* rli = RLI_get_context();
  RLI_Key_state* state = rli->key_state_for_current_frame + key;
  B32 result = (B32)state->is_down;
  return result;
}

U8 u8_from_rli_key(RLI_Key key, B32 is_shift_down, B32* out_is_printable)
{
  static B32 is_initialised = false;
  static struct {
    U8 ch;
    U8 ch_shift;
    B8 printable;
  } key_print_data_arr[RLI_Key__COUNT];

  if (!is_initialised) 
  {
    is_initialised = true;

    for (U32 i = 0; i < RLI_Key__COUNT; i++) {
      key_print_data_arr[i] = {'\0', '\0', false};
    }

    key_print_data_arr[RLI_Key__space]         = {' ',  ' ',  true};
    key_print_data_arr[RLI_Key__a]             = {'a',  'A',  true};
    key_print_data_arr[RLI_Key__b]             = {'b',  'B',  true};
    key_print_data_arr[RLI_Key__c]             = {'c',  'C',  true};
    key_print_data_arr[RLI_Key__d]             = {'d',  'D',  true};
    key_print_data_arr[RLI_Key__e]             = {'e',  'E',  true};
    key_print_data_arr[RLI_Key__f]             = {'f',  'F',  true};
    key_print_data_arr[RLI_Key__g]             = {'g',  'G',  true};
    key_print_data_arr[RLI_Key__h]             = {'h',  'H',  true};
    key_print_data_arr[RLI_Key__i]             = {'i',  'I',  true};
    key_print_data_arr[RLI_Key__j]             = {'j',  'J',  true};
    key_print_data_arr[RLI_Key__k]             = {'k',  'K',  true};
    key_print_data_arr[RLI_Key__l]             = {'l',  'L',  true};
    key_print_data_arr[RLI_Key__m]             = {'m',  'M',  true};
    key_print_data_arr[RLI_Key__n]             = {'n',  'N',  true};
    key_print_data_arr[RLI_Key__o]             = {'o',  'O',  true};
    key_print_data_arr[RLI_Key__p]             = {'p',  'P',  true};
    key_print_data_arr[RLI_Key__q]             = {'q',  'Q',  true};
    key_print_data_arr[RLI_Key__r]             = {'r',  'R',  true};
    key_print_data_arr[RLI_Key__s]             = {'s',  'S',  true};
    key_print_data_arr[RLI_Key__t]             = {'t',  'T',  true};
    key_print_data_arr[RLI_Key__u]             = {'u',  'U',  true};
    key_print_data_arr[RLI_Key__v]             = {'v',  'V',  true};
    key_print_data_arr[RLI_Key__w]             = {'w',  'W',  true};
    key_print_data_arr[RLI_Key__x]             = {'x',  'X',  true};
    key_print_data_arr[RLI_Key__y]             = {'y',  'Y',  true};
    key_print_data_arr[RLI_Key__z]             = {'z',  'Z',  true};
    key_print_data_arr[RLI_Key__0]             = {'0',  ')',  true};
    key_print_data_arr[RLI_Key__1]             = {'1',  '!',  true};
    key_print_data_arr[RLI_Key__2]             = {'2',  '@',  true};
    key_print_data_arr[RLI_Key__3]             = {'3',  '#',  true};
    key_print_data_arr[RLI_Key__4]             = {'4',  '$',  true};
    key_print_data_arr[RLI_Key__5]             = {'5',  '%',  true};
    key_print_data_arr[RLI_Key__6]             = {'6',  '^',  true};
    key_print_data_arr[RLI_Key__7]             = {'7',  '&',  true};
    key_print_data_arr[RLI_Key__8]             = {'8',  '*',  true};
    key_print_data_arr[RLI_Key__9]             = {'9',  '(',  true};
    key_print_data_arr[RLI_Key__backtick]      = {'`',  '~',  true};
    key_print_data_arr[RLI_Key__minus]         = {'-',  '_',  true};
    key_print_data_arr[RLI_Key__equals]        = {'=',  '+',  true};
    key_print_data_arr[RLI_Key__left_bracket]  = {'[',  '{',  true};
    key_print_data_arr[RLI_Key__right_bracket] = {']',  '}',  true};
    key_print_data_arr[RLI_Key__backslash]     = {'\\', '|',  true};
    key_print_data_arr[RLI_Key__semicolon]     = {';',  ':',  true};
    key_print_data_arr[RLI_Key__apostrophe]    = {'\'', '"',  true};
    key_print_data_arr[RLI_Key__comma]         = {',',  '<',  true};
    key_print_data_arr[RLI_Key__period]        = {'.',  '>',  true};
    key_print_data_arr[RLI_Key__slash]         = {'/',  '?',  true};
  }
  
  if (out_is_printable) { *out_is_printable = key_print_data_arr[key].printable; } ;
  U8 ch = key_print_data_arr[key].ch;
  if (is_shift_down) { ch = key_print_data_arr[key].ch_shift; }
  return ch;
}




#endif