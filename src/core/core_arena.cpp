#ifndef BASE_CORE_ARENA_CPP
#define BASE_CORE_ARENA_CPP

#include "core/core_arena.h"
#include "os/win32.cpp"

#define ArenaMinAllocSize 1024 * 4 
StaticAssert(ArenaMinAllocSize > sizeof(Arena)); // This is for meta data just in case

// #ifndef __ArenaReserveMemChunk
// StaticAssert(false, "Define from the os layer must be made to compile arena");
// #endif

// #ifndef __ArenaCommitMemPagesToChunk
// StaticAssert(false, "Define from the os layer must be made to compile arena");
// #endif

// #ifndef __ ArenaDecommitMemPagesFromChunk
// StaticAssert(false, "Define from the os layer must be made to compile arena");
// #endif

// #ifndef __ArenaReleaseMemChunk
// StaticAssert(false, "Define from the os layer must be made to compile arena");
// #endif

///////////////////////////////////////////////////////////
// - arena stuff
//
Arena* arena_alloc(U64 size_to_reserve, B32 start_at_specific_page, U32 allocation_granulatity_index)
{
  if (size_to_reserve < ArenaMinAllocSize) { size_to_reserve = ArenaMinAllocSize; }

  U64 n_pages_to_reserve = ((size_to_reserve + __arena_g_page_size - 1) / __arena_g_page_size);

  Mem_chunk mem_chunk = os_reserve_mem_chunk(n_pages_to_reserve, start_at_specific_page, allocation_granulatity_index);
  if (IsZeroStruct(mem_chunk)) { InvalidCodePath(); exit(1); }

  B32 commit_succ = os_commit_mem_pages_to_chunk(&mem_chunk, 1);
  if (!commit_succ) { InvalidCodePath(); exit(1); }

  Arena* arena = (Arena*)mem_chunk.base_p;
  arena->mem_chunck    = mem_chunk;
  arena->metadata_size = sizeof(Arena);
  arena->bytes_used    = sizeof(Arena);
  return arena;
}

void arena_release(Arena** arena)
{
  B32 succ = os_release_mem_chunk(&(*arena)->mem_chunck);
  *arena = 0; 
  if (!succ) { BP; }
}

U8* arena_push_nozero(Arena* arena, U64 size_to_push)
{
  if (arena->bytes_used + size_to_push > arena->mem_chunck.n_pages_commited * __arena_g_page_size)
  {
    U64 bytes_we_have_place_for      = (arena->mem_chunck.n_pages_commited * __arena_g_page_size) - arena->bytes_used;
    U64 bytes_we_dont_have_place_for = size_to_push - bytes_we_have_place_for;

    U64 pages_needed        = (bytes_we_dont_have_place_for / __arena_g_page_size) + 1;
    U64 pages_we_can_commit = arena->mem_chunck.n_pages_reserved - arena->mem_chunck.n_pages_commited;

    B32 succ = false;
    if (pages_we_can_commit >= pages_needed)
    {
      succ = os_commit_mem_pages_to_chunk(&arena->mem_chunck, pages_needed);
    }
    if (!succ) { BP; exit(1); }
  }

  U8* result_p = (U8*)arena->mem_chunck.base_p + arena->bytes_used;
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
  B32 result = arena->bytes_used == (arena->mem_chunck.n_pages_commited * os_get_mem_page_size());
  return result;
}

B32 arena_can_fit(Arena* arena, U64 size)
{
  U64 bytes_left = (arena->mem_chunck.n_pages_commited * os_get_mem_page_size()) - arena->bytes_used;
  B32 result = (bytes_left >= size);
  return result; 
}

// todo: Make sure indexes here are fine and dont f up on edge cases
U64 arena_get_pos(Arena* arena)
{
  return arena->bytes_used;
}

// todo: Test this 
void arena_pop_to_pos(Arena* arena, U64 new_arena_pos)
{
  if (new_arena_pos > arena->bytes_used) { Assert(false); return; }

  if (new_arena_pos < arena->metadata_size) { new_arena_pos = arena->metadata_size; }
  arena->bytes_used = new_arena_pos;
}

void arena_pop(Arena* arena, U64 bytes_to_pop)
{
  if (bytes_to_pop > arena->bytes_used - arena->metadata_size)
  {
    bytes_to_pop = arena->bytes_used - arena->metadata_size;
  }
  arena->bytes_used -= bytes_to_pop;
}

void arena_clear(Arena* arena)
{
  if (arena == 0) { Assert(0); return; }
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
  if (temp->arena == 0) { InvalidCodePath("This shoud no happend, but it doesnt break the code, so dev time assert is fine"); return;  }

  temp->arena->bytes_used = temp->stored_index;
  *temp = {};
}



#endif





