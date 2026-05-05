#ifndef BASE_CORE_ARENA_CPP
#define BASE_CORE_ARENA_CPP

#include "core/core_arena.h"

///////////////////////////////////////////////////////////
// - arena stuff
//
Arena* arena_alloc(U64 size_to_alloc)
{
  if (size_to_alloc < ArenaMinAllocSize)
  {
    size_to_alloc = ArenaMinAllocSize;
  }
  U8* mem = (U8*)malloc(size_to_alloc);
  if (!mem)
  {
    BreakPoint();
    printf("Arena allocation failed, out of memory for malloc i guess.");
    exit(1);
  }
  Arena* arena = (Arena*)mem;
  arena->base_p = mem;
  arena->bytes_allocated = size_to_alloc;
  arena->metadata_size = sizeof(Arena); 
  arena->bytes_used = arena->metadata_size;
  return arena;
}

void arena_release(Arena* arena)
{
  free(arena->base_p);
}

U8* arena_push_nozero(Arena* arena, U64 size_to_push)
{
  // note: I have houd this too happend some times when you write some code with state for the first time.
  //       You forget to init the arena. Debuggers dont show this very well, so i just added this.
  if (arena == 0) { BreakPoint(); } 

  if (arena->bytes_used + size_to_push > arena->bytes_allocated)
  {
    BreakPoint();
    printf("Attempted a push onto arena and there was no more memory on the arena left.");
    exit(1);
  }
  U8* result_p = arena->base_p + arena->bytes_used;
  arena->bytes_used += size_to_push;
  return result_p;
}

U8* arena_push(Arena* arena, U64 size_to_push)
{
  U8* mem = arena_push_nozero(arena, size_to_push);
  memset(mem, 0, size_to_push);
  return mem;
}

B32 arena_is_consumed(Arena* arena)
{
  B32 result = arena->bytes_used == arena->bytes_allocated;
  return result;
}

B32 arena_can_fit(Arena* arena, U64 size)
{
  U64 bytes_left = arena->bytes_allocated - arena->bytes_used;
  B32 result = (bytes_left >= size);
  return result; 
}

U64 arena_get_bytes_used(Arena* arena)
{
  return arena->bytes_used;
}

// todo: Make sure indexes here are fine and dont f up on edge cases
void arena_pop(Arena* arena, U64 bytes_to_pop)
{
  if (bytes_to_pop > arena->bytes_used - arena->metadata_size)
  {
    bytes_to_pop = arena->bytes_used - arena->metadata_size;
  }
  arena->bytes_used -= bytes_to_pop;
}

// todo: Test this 
void arena_pop_to(Arena* arena, U64 new_arena_pos)
{
  if (new_arena_pos > arena->bytes_used) { Assert(false); return; }

  if (new_arena_pos < arena->metadata_size) { new_arena_pos = arena->metadata_size; }
  arena->bytes_used = new_arena_pos;
}

void arena_clear(Arena* arena)
{
  arena->bytes_used = arena->metadata_size;
}

///////////////////////////////////////////////////////////
// - temp arena stuff
//
Temp_arena temp_arena_begin(Arena* arena)
{
  Temp_arena temp = {};
  temp.arena = arena;
  temp.stored_index = arena->bytes_used;
  return temp;
}

void temp_arena_end(Temp_arena* temp)
{
  temp->arena->bytes_used = temp->stored_index;
  *temp = {};
}



#endif





