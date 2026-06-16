#ifndef CORE_THREAD_CONTEXT_CPP
#define CORE_THREAD_CONTEXT_CPP

#include "core/core_thread_context.h"

Thread_context* get_thread_context()
{
  return &__per_thread_context;
}

void set_thread_context(Thread_context* other)
{
  __per_thread_context.is_initialised = other->is_initialised;
  for EachIndex(i, ArrayCount(__per_thread_context.scrach_arenas_buffer))
  {
    __per_thread_context.scrach_arenas_buffer[i] = other->scrach_arenas_buffer[i];
  }
}

void allocate_thread_context()
{
  Thread_context* context = get_thread_context();
  Assert(!context->is_initialised);
  context->is_initialised = true;
  for EachIndex(i, ArrayCount(context->scrach_arenas_buffer))
  {
    context->scrach_arenas_buffer[i] = arena_alloc(Gigabytes(64), false, 0);
  }
}

void release_thread_context()
{
  Thread_context* context = get_thread_context();
  Assert(context->is_initialised);
  for EachIndex(i, ArrayCount(context->scrach_arenas_buffer))
  {
    arena_release(&context->scrach_arenas_buffer[i]);
  }
  *context = Thread_context{};
}

// todo: Might want to call this Thread arena
//       Thread_arena thread_arena 
Scratch get_scratch(Arena** conflic_arenas, U64 n_conflict_arenas)
{
  Thread_context* thread_context = get_thread_context();
  
  Arena* arena_to_use = 0;
  for EachIndex(test_arena_index, ArrayCount(thread_context->scrach_arenas_buffer))
  {
    B32 found = true;
    Arena* test_arena = thread_context->scrach_arenas_buffer[test_arena_index];
    for EachIndex(conflict_arena_index, n_conflict_arenas)
    {
      Arena* conflict_arena = conflic_arenas[conflict_arena_index];
      if (test_arena == conflict_arena)
      {
        found = false;
        break;
      }
    }
    if (found) 
    {
      arena_to_use = test_arena;
      break;
    }
  }
  Assert(arena_to_use);
  Scratch scratch = temp_arena_begin(arena_to_use);
  return scratch;
}

void end_scratch(Scratch* scratch)
{
  temp_arena_end(scratch);
}

#endif