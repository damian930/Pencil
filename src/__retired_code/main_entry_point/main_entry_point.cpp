#ifndef MAIN_ENTRY_POINT
#define MAIN_ENTRY_POINT

// note: Not sure how much i like this api here to be honest
/*
#include "core/core_thread_context.h"
#include "core/core_thread_context.cpp"

void EntryPoint();

int main()
{
  // Allocating main thread thread_context  
  allocate_thread_context(); 

  #if OS_LAYER
  os_init();
  #endif
  #if RLI_LAYER
  RLI_init(os_get_keyboard_initial_repeat_delay(), os_get_keyboard_subsequent_repeat_delay());
  #endif
  #if UI_LAYER
  ui_init();
  #endif

  EntryPoint();

  #if OS_LAYER
  os_release();
  #endif
  #if RLI_LAYER
  RLI_release();
  #endif
  #if UI_LAYER
  ui_release();
  #endif

  return 0;
}
*/

#endif