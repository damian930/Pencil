#ifndef CORE_PROFILER_H
#define CORE_PROFILER_H

#include "core/core_base.h"
#include "core/core_string.h"
#include "core/core_thread_context.h"

#include "core/core_base.cpp"
#include "core/core_string.cpp"
#include "core/core_thread_context.cpp"

struct Porofile_record {
  F64 start_time_sec;
  F64 end_time_sec;
  Str8 func_name;
};

struct Profile_state {
  Porofile_record nested_records_arr[256];
  U64 nested_record_count;

  Arena* final_data_arena;
  U8* final_data_arr;
  U64 final_data_byte_count;
};

global Profile_state __g_profile_state = {};

void profile_func_begin(Str8 func_name, F64 (*get_time_for_timing_sec)(void))
{
  if (IsMemZero(__g_profile_state)) {
    __g_profile_state.final_data_arena = arena_alloc(Megabytes(64));
    __g_profile_state.final_data_arr = ArenaCurrentAddressP(__g_profile_state.final_data_arena, U8);
  }

  if (__g_profile_state.nested_record_count >= ArrayCount(__g_profile_state.nested_records_arr)) { InvalidCodePath(); return; }

  Porofile_record* record = __g_profile_state.nested_records_arr + (__g_profile_state.nested_record_count++);
  record->start_time_sec = get_time_for_timing_sec();
  record->func_name = func_name;
}

void profile_func_end(F64 (*get_time_for_timing_sec)(void))
{
  if (IsMemZero(__g_profile_state)) { InvalidCodePath(); return; }

  Porofile_record* record = __g_profile_state.nested_records_arr + (__g_profile_state.nested_record_count - 1);
  record->end_time_sec = get_time_for_timing_sec();

  Scratch scratch     = get_scratch(0, 0);
  Str8 name_nt        = str8_copy_alloc(scratch.arena, record->func_name);
  F64 duration_in_sec = record->end_time_sec - record->start_time_sec;
  Str8 str = str8_fmt(scratch.arena, "[Function profile] \"%s\"::%.3fsec (%.3fms) \n", name_nt.data, duration_in_sec, duration_in_sec * MsInSec);

  ArenaPushArr(__g_profile_state.final_data_arena, U8, str.count);
  memcpy(__g_profile_state.final_data_arr + __g_profile_state.final_data_byte_count, str.data, str.count);
  __g_profile_state.final_data_byte_count += str.count;

  end_scratch(&scratch);
  __g_profile_state.nested_record_count -= 1;
}

Data_buffer profile_get_data()
{
  return str8_manuall(__g_profile_state.final_data_arr, __g_profile_state.final_data_byte_count);
}

#define ProfileFuncBegin() profile_func_begin(Str8FromC(__FUNCTION__), os_get_time_for_timing_sec);
#define ProfileFuncEnd()   profile_func_end(os_get_time_for_timing_sec);



#endif