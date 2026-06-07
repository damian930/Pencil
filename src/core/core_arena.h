#ifndef BASE_CORE_ARENA_H
#define BASE_CORE_ARENA_H

#include "core/core_base.h"

struct Mem_chunk {
  void* base_p;
  U64 n_pages_reserved;
  U64 n_pages_commited;
};

struct Arena {
  Mem_chunk mem_chunck;
  U64 metadata_size;
  U64 bytes_used;
};

struct Temp_arena {
  Arena* arena;
  U64 stored_index;
};

global U64 __arena_g_page_size = Kilobytes(4);

// - arena stuff
tu_specific Arena* arena_alloc(U64 size_to_reserve, B32 start_at_specific_page, U32 allocation_granulatity_index);
tu_specific void arena_release(Arena** arena);

tu_specific U8* arena_push_nozero(Arena* arena, U64 size_to_push);
tu_specific U8* arena_push(Arena* arena, U64 size_to_push);
#define ArenaPush(arena_p, Type)           ((Type*)arena_push(arena_p, sizeof(Type)))
#define ArenaPushArr(arena_p, Type, count) ((Type*)arena_push(arena_p, sizeof(Type) * count))
#define ArenaCurrentAddressP(arena_p, Type) (ArenaPushArr(arena_p, Type, 0))
#define ArenaCurrentAddressU64(arena_p)     ((U64)ArenaPushArr(arena_p, U8, 0))

tu_specific B32 arena_is_consumed(Arena* arena);
tu_specific B32 arena_can_fit(Arena* arena, U64 size);
#define ArenaCanFit(arena_p, Type)           arena_can_fit(arena_p, sizeof(Type))
#define ArenaCanFitArr(arena_p, Type, count) arena_can_fit(arena_p, sizeof(Type) * count)

tu_specific U64 arena_get_pos(Arena* arena);
tu_specific void arena_pop_to_pos(Arena* arena, U64 new_arena_pos);
tu_specific void arena_pop(Arena* arena, U64 bytes_to_pop);
tu_specific void arena_clear(Arena* arena);
#define ArenaPopType(arena_p, Type) arena_pop(arena_p, sizeof(Type))

// - temp arena stuff
tu_specific Temp_arena temp_arena_begin(Arena* arena);
tu_specific void temp_arena_end(Temp_arena* temp);
typedef Temp_arena Scratch;

#endif





