#ifndef _CORE_STRING_H_
#define _CORE_STRING_H_

#include "string.h" // For emset and strlen

#include "core/core_base.h"
#include "core/core_arena.h"

/* todo:
  [ ] - improve the section for str8_list, right now only 1 can exist.
        api asserts if another one gets created.
  [ ] - use #define CLAY__STRING_LENGTH(s) ((sizeof(s) / sizeof((s)[0])) - sizeof((s)[0])) 
        instead of strlen
  [ ] - search for other todos in the code ))
*/

struct Str8 {
  U8* data;
  U64 count;
};

struct Str8_node {
  Str8 str;
  Str8_node* next;
  Str8_node* prev;
};

struct Str8_list {
  Str8 str;
  Str8_node* first;
  Str8_node* last;
  U64 node_count;
  U64 codepoint_count;
  
  B8 is_temp_section_present;
  U64 nodes_in_temp_section;
  U64 codepoints_in_temp_section;
};  

enum Str8_match : U32 {
  Str8_match__NONE            = 0,
  Str8_match__ignore_case     = (1 << 0),
  Str8_match__normalise_slash = (1 << 1),
};
typedef U32 Str8_match_flags;

// - data buffer
typedef Str8 Data_buffer;
tu_specific Data_buffer data_buffer_make(Arena* arena, U64 count);

// - str8 makers
#define CStrCount(clit) ArrayCount(clit) - 1
#define MakeSureClit(clit) "" clit "" 
#define Str8FromC(clit) Str8 { (U8*)MakeSureClit(clit), ArrayCount(clit) - 1 }
#define String(clit) Str8FromC(clit) // note: Testing a faster macro
tu_specific Str8 str8_manuall(U8* buffer, U64 count);
tu_specific Str8 str8_from_cstr_len(Arena* arena, U8* str, U64 len);
tu_specific Str8 str8_from_cstr(Arena* arena, U8* str);
tu_specific Str8 str8_copy_alloc(Arena* arena, Str8 str);
tu_specific Str8 str8_from_list(Arena* arena, Str8_list* list); // todo: There is not point in the list beeing passed by a pointer here. It shoud either be by value. Or a const pointer
tu_specific Str8 str8_from_list_ex(Arena* arena, const Str8_list list, Str8 str_to_put_before, Str8 str_to_put_between, Str8 str_to_put_after); 

// - substrings
tu_specific Str8 str8_substring(Str8 str, U64 start_index, U64 end_index);
tu_specific Str8 str8_substring_range(Str8 str, RangeU64 range);
tu_specific B32 str8_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific B32 str8_is_substring(Str8 str, Str8 test_sub, Str8_match_flags flags);
tu_specific B32 str8_is_front(Str8 str, Str8 other);
tu_specific B32 str8_is_back(Str8 str, Str8 other);
tu_specific Str8 str8_chop_front(Str8 str, U64 end_index);
tu_specific Str8 str8_chop_back(Str8 str, U64 n_chars_to_chop);
tu_specific Str8 str8_chop_front_if_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific Str8 str8_chop_back_if_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific Str8 str8_front(Str8 str, U64 n);
tu_specific Str8 str8_back(Str8 str, U64 n);
tu_specific Str8 str8_trim_front(Str8 str);
tu_specific Str8 str8_trim_back(Str8 str);
tu_specific Str8 str8_trim(Str8 str);

// - chars
tu_specific B32 char_is_lower(U8 ch);
tu_specific B32 char_is_upper(U8 ch);
tu_specific U8 char_to_lower(U8 ch); 
tu_specific U8 char_to_upper(U8 ch); 
tu_specific U8 char_normalise_slash(U8 ch);
tu_specific B32 char_is_alpha(U8 ch);
tu_specific B32 char_is_numeric(U8 ch);
tu_specific B32 char_is_alnum(U8 ch);
tu_specific B32 char_is_word_char(U8 ch);

// - list stuff
tu_specific Str8_list str8_list_copy_shallow(Arena* arena, Str8_list other);
tu_specific Str8_list str8_list_copy_deep(Arena* arena, Str8_list other);
tu_specific void str8_list_append(Arena* arena, Str8_list* list, Str8 str);
tu_specific void str8_list_append_copy(Arena* arena, Str8_list* list, Str8 str); // todo: The name for this is not great at all
tu_specific Str8_list str8_split(Arena* arena, Str8 str, Str8 spliter, Str8_match_flags flags);
tu_specific void str8_list_begin_temp_section(Str8_list* list); // todo: Fix these up, only 1 section is possible at a time
tu_specific void str8_list_end_temp_section(Str8_list* list);
#define Str8ListSection(list_p) DeferLoop(str8_list_begin_temp_section((list_p)), str8_list_end_temp_section((list_p)))

// - other
tu_specific Str8 str8_get_filename(Str8 str);
tu_specific Str8 str8_get_basename(Str8 str);
tu_specific Str8 str8_chop_extenstion(Str8 str);
tu_specific Str8 str8_from_month(Month month);
tu_specific Str8 str8_from_day(Day day);
tu_specific Comparison str8_compare(Str8 str, Str8 other, Str8_match_flags flags);

///////////////////////////////////////////////////////////
// - fmt (public) 
// todo: These 2 dont account for max size of u64/s64
tu_specific B32 str8_is_u64(Str8 str);
tu_specific B32 str8_is_s64(Str8 str);

tu_specific U64 u64_as_str8_size(U64 v);
tu_specific U64 s64_as_str8_size(S64 v);
tu_specific U64 pointer_as_str8_size(void* p);
tu_specific U64 f64_as_str8_size(F64 v);
tu_specific U64 b64_as_str_size(B64 b);

// - fmt main api
tu_specific U64 str8_size_for_formated_str(const char* fmt, ...);
tu_specific Str8 str8_from_fmt_dyn(Arena* arena, const char* fmt, ...);
tu_specific U64 str8_from_fmt_s(Data_buffer* out_buffer, const char* fmt, ...);
tu_specific void str8_printf(const char* fmt, ...);
// note: These are not thread safe, so dont use them in a thread pool or something

///////////////////////////////////////////////////////////
// - fmt (private)
enum __Str8_fmt_token_kind : U32 {
  __Str8_fmt_token_kind__NONE,

  __Str8_fmt_token_kind__U64,
  __Str8_fmt_token_kind__U,

  __Str8_fmt_token_kind__S64,
  __Str8_fmt_token_kind__S,

  __Str8_fmt_token_kind__B64,
  __Str8_fmt_token_kind__B,

  __Str8_fmt_token_kind__Pointer,
  __Str8_fmt_token_kind__Str8,

  __Str8_fmt_token_kind__other,
};

struct __Str8_fmt_token {
  __Str8_fmt_token_kind kind;
  Str8 str;
};

struct __Str8_fmt_lexer {
  U64 token_start_pos;
  U64 current_pos;
  Str8 original_str;
};  

// - fmt lexer
tu_specific __Str8_fmt_token __str8_fmt_lexer_eat_token(__Str8_fmt_lexer* lexer);
tu_specific B32 __str8_fmt_lexer_is_done(const __Str8_fmt_lexer* lexer);
tu_specific U8 __str8_fmt_lexer_eat_char(__Str8_fmt_lexer* lexer);
tu_specific U8 __str8_fmt_lexer_peek_char(const __Str8_fmt_lexer* lexer);
tu_specific B32 __str8_fmt_lexer_match_char(__Str8_fmt_lexer* lexer, U8 ch);
tu_specific B32 __str8_fmt_lexer_match_str(__Str8_fmt_lexer* lexer, Str8 str);
tu_specific __Str8_fmt_token __str8_fmt_lexer_make_token(__Str8_fmt_lexer* lexer, __Str8_fmt_token_kind kind);

// - fmt buffer manipulations
tu_specific U64 __str8_fmt_size_for_formatted_str_or_fill_buffer(const char* fmt, va_list* va_list, Data_buffer* out_opt_buffer);
tu_specific U64 __str8_fmt_put_u64_into_buffer(U8* buffer, U64 buffer_size, U64 v);
tu_specific U64 __str8_fmt_put_s64_into_buffer(U8* buffer, U64 buffer_size, S64 v);
tu_specific U64 __str8_fmt_put_str_into_buffer(U8* buffer, U64 buffer_size, Str8 str);
tu_specific U64 __str8_fmt_put_b64_into_buffer(U8* buffer, U64 buffer_size, B64 b);



#endif 