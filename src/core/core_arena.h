#ifndef BASE_CORE_ARENA_H
#define BASE_CORE_ARENA_H

#include "stdlib.h"         // For malloc
#include "string.h"         // For memset
#include "core/core_base.h"

struct Arena {
  U8* base_p;
  U64 bytes_allocated;
  U64 metadata_size;
  U64 bytes_used;
};

struct Temp_arena {
  Arena* arena;
  U64 stored_index;
};

#define ArenaMinAllocSize 1024 * 4

// I cant seem to find a good way to handle if the user or arena_alloc passes in a small value that 
// the sizeof(arena) itself cant be allocated. I could have a callback thing set up that gets implemented
// by the user, so if something fucks up, then something happends, but i am not sure about that. 
// I dont want to just have to check for not being able to allocate in code.
// If arena fails, this does mean by design that the arena size that got preallocated is wrong and 
// therefore shoud be changed. But i also dont want to check for fail on arena creation ever.
// This is not an issue when using VAlloc, cause it has a min allocation that is 4Kb. 
// I am going to use something similar, Min size for allocations for arenas.

// - arena stuff
tu_specific Arena* arena_alloc(U64 size_to_alloc);
tu_specific void arena_release(Arena* arena);

tu_specific U8* arena_push_nozero(Arena* arena, U64 size_to_push);
tu_specific U8* arena_push(Arena* arena, U64 size_to_push);
#define ArenaPush(arena_p, Type)           (Type*)arena_push(arena_p, sizeof(Type))
#define ArenaPushArr(arena_p, Type, count) (Type*)arena_push(arena_p, count * sizeof(Type))
#define ArenaCurrentPos(arena_p, Type)     ArenaPushArr(arena_p, Type, 0)

tu_specific B32 arena_is_consumed(Arena* arena);
tu_specific B32 arena_can_fit(Arena* arena, U64 size);
#define ArenaCanFit(arena_p, Type) arena_can_fit(arena_p, sizeof(Type))

tu_specific void arena_pop(Arena* arena, U64 bytes_to_pop);
tu_specific void arena_clear(Arena* arena);

// - temp arena stuff
tu_specific Temp_arena temp_arena_begin(Arena* arena);
tu_specific void temp_arena_end(Temp_arena* temp);
typedef Temp_arena Scratch;

#endif





