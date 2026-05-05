#ifndef _CORE_STRING_CPP_
#define _CORE_STRING_CPP_

#include "core_string.h"
#include "core_thread_context.cpp"

///////////////////////////////////////////////////////////
// - data buffer
//
Data_buffer data_buffer_make(Arena* arena, U64 count)
{
  // note: If len is 0 here, sure we get a pointer onto an arena 
  //       that then might also be used for something else,
  //       but its fine, since str.count will be 0, so we know not to index into it.
  Data_buffer buffer = {};
  buffer.data = ArenaPushArr(arena, U8, count);
  buffer.count = count;
  return buffer;
}


///////////////////////////////////////////////////////////
// - makers 
// 
Str8 str8_manuall(U8* buffer, U64 count)
{
  Str8 str = {};
  str.data = buffer;
  str.count = count;
  return str;
}

Str8 str8_from_cstr_len(Arena* arena, U8* str, U64 len)
{
  // note: If len is 0 here, sure we get a pointer onto an arena 
  //       that then might also be used for something else,
  //       but its fine, since str.count will be 0, so we know not to index into it.
  Str8 result = {};
  result = data_buffer_make(arena, len);
  memcpy(result.data, str, len);
  U8* nt = ArenaPush(arena, U8);
  *nt = '\0';
  return result;
}

Str8 str8_from_cstr(Arena* arena, U8* str)
{
  Str8 result = str8_from_cstr_len(arena, str, strlen((char*)str));
  return result;
}

Str8 str8_copy_alloc(Arena* arena, Str8 str)
{
  Str8 copy = str8_from_cstr_len(arena, str.data, str.count);
  return copy;
}

Str8 str8_from_list(Arena* arena, Str8_list* list)
{
  Str8 str = str8_from_list_ex(arena, *list, Str8{}, Str8{}, Str8{});
  return str;
}

Str8 str8_from_list_ex(Arena* arena, const Str8_list list, Str8 str_to_put_before, Str8 str_to_put_between, Str8 str_to_put_after)
{
  Str8 str = str8_manuall(ArenaCurrentAddressP(arena, U8), 0);
  U64 node_index = 0;

  if (str_to_put_before.count > 0)
  {
    U8* str_part = ArenaPushArr(arena, U8, str_to_put_before.count);
    memcpy(str_part, str_to_put_before.data, str_to_put_before.count);
    str.count += str_to_put_before.count;
  }

  for (Str8_node* node = list.first; node != 0; node = node->next)
  {
    U8* str_part = ArenaPushArr(arena, U8, node->str.count);
    memcpy(str_part, node->str.data, node->str.count);
    str.count += node->str.count;

    if (str_to_put_between.count > 0 
      && 0 <= node_index && node_index < list.node_count - 1
    ) {
      U8* in_between_part = ArenaPushArr(arena, U8, str_to_put_between.count);
      memcpy(in_between_part, str_to_put_between.data, str_to_put_between.count);
      str.count += str_to_put_between.count;
    }
    node_index += 1;
  }

  if (str_to_put_after.count > 0)
  {
    U8* str_part = ArenaPushArr(arena, U8, str_to_put_after.count);
    memcpy(str_part, str_to_put_after.data, str_to_put_after.count);
    str.count += str_to_put_after.count;
  }

  U8* nt = ArenaPush(arena, U8);
  *nt = '\0';
  return str;
}

///////////////////////////////////////////////////////////
// - substring 
// 
Str8 str8_substring(Str8 str, U64 start_index, U64 end_index)
{
  if (end_index > str.count)
  {
    end_index = str.count;
  }
  Str8 result = {};
  if (start_index < end_index)
  {
    result.data = str.data + start_index;
    result.count = end_index - start_index;
  }
  return result;
}

Str8 str8_substring_range(Str8 str, RangeU64 range)
{
  return str8_substring(str, range.min, range.max);
}

B32 str8_match(Str8 str, Str8 other, Str8_match_flags flags)
{
  B32 is_match = (str.count == other.count);
  if (is_match)
  {
    for (U64 i = 0; i < str.count; i += 1)
    {
      U8 str_ch = str.data[i];
      U8 other_ch = other.data[i];
      if (flags & Str8_match__ignore_case)
      {
        str_ch   = char_to_lower(str_ch);
        other_ch = char_to_lower(other_ch);
      }
      if (flags & Str8_match__normalise_slash)
      {
        str_ch   = char_normalise_slash(str_ch);
        other_ch = char_normalise_slash(other_ch);
      }
     
      if (str_ch != other_ch)
      {
        is_match = false;
        break;
      }
    }
  }
  return is_match;
}

B32 str8_is_substring(Str8 str, Str8 test_sub, Str8_match_flags flags)
{
  B32 is_sub = false;
  for (U64 i = 0; i < str.count; i += 1)
  {
    if (i + test_sub.count < str.count)
    {
      Str8 str_sub = str8_substring(str, i, i + test_sub.count);
      if (str8_match(str_sub, test_sub, flags))
      {
        is_sub = true;
        break;
      }
    }
    else 
    {
      break;
    }
  }
  return is_sub;
}

B32 str8_is_front(Str8 str, Str8 other)
{
  if (str.count == 0) { return false; }
  if (str.count < other.count) { return false; }
  Str8 test_str = str8_substring(str, 0, other.count);
  B32 result = str8_match(test_str, other, 0);
  return result;
}

B32 str8_is_back(Str8 str, Str8 other)
{
  if (str.count == 0) { return false; }
  if (str.count < other.count) { return false; }
  Str8 test_str = str8_substring(str, str.count - other.count, str.count);
  B32 result = str8_match(test_str, other, 0);
  return result;
}

Str8 str8_chop_front(Str8 str, U64 n_chars_to_chop)
{
  Str8 result = str8_substring(str, n_chars_to_chop, str.count);
  return result;
}

Str8 str8_chop_back(Str8 str, U64 n_chars_to_chop)
{
  Str8 result = {};
  if (str.count >= n_chars_to_chop) // Making sure we dont overflow back U64 if otherwise
  {
    result = str8_substring(str, 0, str.count - n_chars_to_chop);
  }
  return result;
}

Str8 str8_chop_front_if_match(Str8 str, Str8 other, Str8_match_flags flags)
{
  Str8 result = str;
  Str8 test_sub = str8_substring(str, 0, other.count);
  if (str8_match(test_sub, other, flags))
  {
    result = str8_chop_front(str, other.count);
  }
  return result;
}

Str8 str8_chop_back_if_match(Str8 str, Str8 other, Str8_match_flags flags)
{
  Str8 result = str;
  if (str.count >= other.count)
  {
    U64 start_index = str.count - other.count;
    Str8 test_sub = str8_substring(str, start_index, str.count);
    if (str8_match(test_sub, other, flags))
    {
      result = str8_chop_back(str, other.count);
    }
  }
  return result;
}

Str8 str8_front(Str8 str, U64 n)
{
  Str8 sub = str8_substring(str, 0, n);
  return sub;
}

Str8 str8_back(Str8 str, U64 n)
{
  if (n > str.count) { n = str.count; }
  Str8 sub = str8_substring(str, str.count - n, str.count);
  return sub;
}

Str8 str8_trim_front(Str8 str)
{
  for (;;)
  {
    if (str.count == 0) { break; }
    if (str.data[0] != ' ') { break; }
    str.data += 1;
    str.count -= 1;
  }
  if (str.count == 0) { str.data = 0; }
  return str;
}

Str8 str8_trim_back(Str8 str)
{
  for (;;)
  {
    if (str.count == 0) { break; }
    if (str.data[str.count - 1] != ' ') { break; }
    str.count -= 1;
  }
  if (str.count == 0) { str.data = 0; }
  return str;
}

Str8 str8_trim(Str8 str)
{
  str = str8_trim_front(str);
  str = str8_trim_back(str);
  return str;
}


// todo: When done with this, move it to a proper place
static inline RangeU64 str8_find(Str8 haystack, Str8 needle, Str8_match_flags match_flags) // notes: I asked claude for param names, it suggested me these
{
  if (haystack.count < needle.count) { return RangeU64{}; }

  // note: we do haystack_index <= haystack.count - needle.count and not just <
  //       because this way we will catch the needle if it is in the end of hay stack.
  //       the start index will be on the needle start and the end will be after the neddle end. (out of bounds)
  //       since ranges are end exclusive, we are good       
  RangeU64 range = {};  
  for (U64 haystack_index = 0; haystack_index <= haystack.count - needle.count; haystack_index += 1)
  {
    Str8 test_str = str8_substring(haystack, haystack_index, haystack_index + needle.count);
    if (str8_match(needle, test_str, match_flags))
    {
      range.min = haystack_index;
      range.max = haystack_index + needle.count;
      break;
    }
  }

  return range;
}



///////////////////////////////////////////////////////////
// - chars
//
B32 char_is_lower(U8 ch)
{
  B32 result = ('a' <= ch && ch <= 'z');
  return result;
}

B32 char_is_upper(U8 ch)
{
  B32 result = ('A' <= ch && ch <= 'Z');
  return result;
}

U8 char_to_lower(U8 ch)
{
  U8 result = ch;
  if (char_is_upper(result))
  {
    result += ('a' - 'A');
  }
  return result;
}

U8 char_to_upper(U8 ch)
{
  U8 result = ch;
  if (char_is_lower(result))
  {
    result -= ('a' - 'A');
  }
  return result;
}

U8 char_normalise_slash(U8 ch)
{
  if (ch == '\\') 
  {
    ch = '/';
  }
  return ch;
}

B32 char_is_alpha(U8 ch)
{
  B32 result = (char_is_lower(ch) || char_is_upper(ch));
  return result;
}

B32 char_is_numeric(U8 ch)
{
  B32 result = ('0' <= ch && ch <= '9');
  return result;
}

B32 char_is_alnum(U8 ch)
{
  B32 result = (char_is_alpha(ch) || char_is_numeric(ch));
  return result;
}

B32 char_is_word_char(U8 ch)
{
  B32 result = (char_is_alnum(ch) || ch == '_');
  return result;
}

///////////////////////////////////////////////////////////
// -list stuff
//
Str8_list str8_list_copy_shallow(Arena* arena, Str8_list other)
{
  Str8_list list = {};
  for (Str8_node* node = other.first; node; node = node->next)
  {
    str8_list_append(arena, &list, node->str);
  }
  return list;
}

Str8_list str8_list_copy_deep(Arena* arena, Str8_list other)
{
  Str8_list list = {};
  for (Str8_node* node = other.first; node; node = node->next)
  {
    str8_list_append_copy(arena, &list, node->str);
  }
  return list;
}

void str8_list_append(Arena* arena, Str8_list* list, Str8 str) 
{
  Str8_node* node = ArenaPush(arena, Str8_node);
  node->str = str;
  DllPushBack(list, node);
  list->node_count += 1;
  list->codepoint_count += str.count;

  if (list->is_temp_section_present)
  {
    list->nodes_in_temp_section += 1;
    list->codepoints_in_temp_section += str.count;
  }
}

void str8_list_append_copy(Arena* arena, Str8_list* list, Str8 str)
{
  Str8 copy = str8_copy_alloc(arena, str);
  str8_list_append(arena, list, copy);
}

Str8_list str8_split(Arena* arena, Str8 str, Str8 spliter, Str8_match_flags flags)
{
  Str8_list result_list = {};
  if (spliter.count <= str.count)
  {
    U64 word_start_index = 0;
    for (U64 i = 0; (i < str.count) && (i + spliter.count <= str.count); i += 1)
    {
      Str8 test_substring = str8_substring(str, i, i + spliter.count);
      if (str8_match(test_substring, spliter, flags))
      {
        Str8 new_substring = str8_substring(str, word_start_index, i);
        if (new_substring.count > 0)
        {
          str8_list_append(arena, &result_list, new_substring);
        }
        word_start_index = i + spliter.count;
      }  
    }
    if (word_start_index < str.count - 1)
    {
      Str8 new_substring = str8_substring(str, word_start_index, str.count);
      str8_list_append(arena, &result_list, new_substring);
    }
  } else {
    str8_list_append(arena, &result_list, str);
  }
  return result_list;
}

void str8_list_begin_temp_section(Str8_list* list)
{
  // The first assert is not handled for now, cause i am just testing this api for now
  // Data: added this section thing on 4th of Feb 2026
  Assert(list->is_temp_section_present == 0);
  Assert(list->nodes_in_temp_section == 0);
  Assert(list->codepoints_in_temp_section == 0);
  list->is_temp_section_present = true;
}

void str8_list_end_temp_section(Str8_list* list)
{
  Assert(list->is_temp_section_present, 
    "If temp section is present there will be no issue, this assert is here mostly to signal that ",
    "there is a place in code where end_section is called when there is no active section, which is weird."
  );
  for (U64 i = 0; i < list->nodes_in_temp_section; i += 1)
  {
    list->codepoint_count -= list->last->str.count;
    list->node_count -= 1;
    DllPopBack(list);
  }
  list->is_temp_section_present    = false;
  list->codepoints_in_temp_section = 0;
  list->nodes_in_temp_section      = 0;
}

///////////////////////////////////////////////////////////
// - other
//
Str8 str8_get_filename(Str8 str)
{
  Scratch scratch = get_scratch(0, 0);
  Str8_list list = str8_split(scratch.arena, str, Str8FromC("/"), Str8_match__normalise_slash);
  Str8 filename = {};
  if (list.node_count > 0)
  { 
    filename = list.last->str;
  }
  end_scratch(&scratch);
  return filename;
}

Str8 str8_get_basename(Str8 str)
{
  Str8 filename = str8_get_filename(str);
  Str8 basename = str8_chop_extenstion(filename);
  return basename;
}

Str8 str8_chop_extenstion(Str8 str)
{
  if (str.count == 0) { return Str8{}; }

  U64 index_for_dot = str.count;
  for (U64 i = str.count - 1; i >= 0; i -= 1)
  {
    if (str.data[i] == '.')
    {
      index_for_dot = i;
      break;
    }
  }
  Str8 result = str8_substring(str, 0, index_for_dot);
  return result;
}

Str8 str8_from_month(Month month)
{ 
  Str8 result = {};
  switch (month)
  {
    default: { Assert(false); } break;
    case Month__january   : { result = Str8FromC("January"); } break;
    case Month__february  : { result = Str8FromC("February"); } break; 
    case Month__march     : { result = Str8FromC("March"); } break;
    case Month__april     : { result = Str8FromC("April"); } break;
    case Month__may       : { result = Str8FromC("May"); } break;
    case Month__june      : { result = Str8FromC("June"); } break;
    case Month__july      : { result = Str8FromC("July"); } break;
    case Month__august    : { result = Str8FromC("August"); } break;
    case Month__september : { result = Str8FromC("September"); } break;
    case Month__october   : { result = Str8FromC("October"); } break;
    case Month__november  : { result = Str8FromC("November"); } break;
    case Month__december  : { result = Str8FromC("December"); } break;
    case Month__COUNT     : {  } break;
  }
  return result;
}

Str8 str8_from_day(Day day)
{
  Str8 result = {};
  switch (day)
  {
    default: { Assert(false); } break;
    case Day__monday    : { result = Str8FromC("Monday"); } break;
	  case Day__tuesday   : { result = Str8FromC("Tuesday"); } break;
	  case Day__wednesday : { result = Str8FromC("Wednesday"); } break;
	  case Day__thursday  : { result = Str8FromC("Thursday"); } break;
	  case Day__frinday   : { result = Str8FromC("Frinday"); } break; 
	  case Day__saturdan  : { result = Str8FromC("Saturdan"); } break; 
	  case Day__sunday    : { result = Str8FromC("Suday"); } break;
	  case Day__COUNT     : {  } break;
  }
  return result;
}

Comparison str8_compare(Str8 str, Str8 other, Str8_match_flags flags)
{
  Comparison result = Comparison__equal;
  for (U64 i = 0; i < Min(str.count, other.count) && result != Comparison{}; i += 1)
  {
    U8 str_ch   = str.data[i];
    U8 other_ch = other.data[i]; 

    if (flags & Str8_match__ignore_case)
    {
      str_ch   = char_to_lower(str_ch);
      other_ch = char_to_lower(other_ch);
    }
    if (flags & Str8_match__normalise_slash)
    { 
      str_ch   = char_normalise_slash(str_ch);
      other_ch = char_normalise_slash(other_ch);
    }

    if (str_ch < other_ch)
    {
      result = Comparison__smaller;
      break;
    }
    else if (str_ch > other_ch)
    {
      result = Comparison__greater;
      break;
    }
  }
  if (result == Comparison__equal)
  {
    if (str.count < other.count)  {
      result = Comparison__smaller;
    } else if (str.count > other.count) {
      result = Comparison__greater;
    }
  }
  return result;
}


///////////////////////////////////////////////////////////
// - fmt (public)
//
B32 str8_is_u64(Str8 str)
{
  if (str.count == 0) { return false; }
  B32 is = true;
  for EachIndex(i, str.count)
  {
    if (!char_is_numeric(str.data[i]))
    {
      is = false;
      break;
    }
  } 
  return is;
}

B32 str8_is_s64(Str8 str)
{
  if (str.count == 0) { return false; }
  Str8 u64_str = str8_chop_front_if_match(str, Str8FromC("-"), 0);
  B32 result = str8_is_u64(u64_str);
  return result;
}

U64 u64_as_str8_size(U64 v)
{
  if (v == 0) { return 1; } 
  U64 counter = 0;
  for (;v > 0;)
  {
    v /= 10;
    counter += 1;
  }
  return counter;
}

U64 s64_as_str8_size(S64 v)
{
  U64 counter = 0;
  if (v >= 0) {
    counter = u64_as_str8_size((U64)v);
  } else {
    v *= -1;
    counter += 1;
    counter += u64_as_str8_size((U64)v);
  }
  return counter;
}

U64 pointer_as_str8_size(void* p)
{
  U64 counter = u64_as_str8_size((U64)p);
  return counter;
}

U64 f64_as_str8_size(F64 v)
{
  (void)v;
  NotImplemented();
  return 0;
}

U64 b64_as_str_size(B64 b)
{
  U64 size = Str8FromC("true").count;
  if (!b) { size = Str8FromC("false").count; }
  return size;
}

// - fmt main api
U64 str8_size_for_formated_str(const char* fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  U64 size = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, Null);
  va_end(argptr);
  return size;
}

Str8 str8_from_fmt_dyn(Arena* arena, const char* fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  U64 size_for_buffer = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, Null);
  va_end(argptr);

  va_start(argptr, fmt);
  Data_buffer buffer = data_buffer_make(arena, size_for_buffer);
  U64 bytes_written = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, &buffer);
  Assert(bytes_written == size_for_buffer);
  va_end(argptr);
  return buffer;
}

U64 str8_from_fmt_s(Data_buffer* out_buffer, const char* fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  U64 size = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, Null);
  va_end(argptr);
  (void)size;

  va_start(argptr, fmt);
  U64 bytes_written = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, out_buffer);
  va_end(argptr);
  return bytes_written;
}

void str8_printf(const char* fmt, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list argptr;
  
  va_start(argptr, fmt);
  U64 size_for_fmt_result_str = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, Null);
  va_end(argptr);
  
  va_start(argptr, fmt);
  Data_buffer buffer = data_buffer_make(scratch.arena, size_for_fmt_result_str);
  U64 bytes_written = __str8_fmt_size_for_formatted_str_or_fill_buffer(fmt, &argptr, &buffer);
  Assert(bytes_written == buffer.count);
  va_end(argptr);

  U8* nt = ArenaPush(scratch.arena, U8);
  *nt = '\0';
  printf("%s", buffer.data);

  end_scratch(&scratch);
}

///////////////////////////////////////////////////////////
// - fmt (private)
//
// - fmt lexer
__Str8_fmt_token __str8_fmt_lexer_eat_token(__Str8_fmt_lexer* lexer)
{
  __Str8_fmt_token result_token = {};
  if (__str8_fmt_lexer_is_done(lexer)) { return result_token; }

  if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#U64"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__U64);
  }
  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#U"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__U);
  }

  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#Str8")))
  {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__Str8);
  }
  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#S64"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__S64);
  }
  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#S"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__S);
  }

  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#B64"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__B64);
  }
  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#B"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__B);
  }
  else if (__str8_fmt_lexer_match_str(lexer, Str8FromC("#P"))) {
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__Pointer);
  }
  
  else {
    __str8_fmt_lexer_eat_char(lexer);
    result_token = __str8_fmt_lexer_make_token(lexer, __Str8_fmt_token_kind__other);
  }
  return result_token;
}

B32 __str8_fmt_lexer_is_done(const __Str8_fmt_lexer* lexer)
{
  B32 result = (lexer->current_pos >= lexer->original_str.count);
  return result;
}

U8 __str8_fmt_lexer_eat_char(__Str8_fmt_lexer* lexer)
{
  U8 ch = '\0';
  if (!__str8_fmt_lexer_is_done(lexer))
  {
    ch = lexer->original_str.data[lexer->current_pos++];
  }
  return ch;
}

U8 __str8_fmt_lexer_peek_char(const __Str8_fmt_lexer* lexer)
{
  U8 ch = '\0';
  if (__str8_fmt_lexer_is_done(lexer))
  {
    ch = lexer->original_str.data[lexer->current_pos];
  }
  return ch;
}

B32 __str8_fmt_lexer_match_char(__Str8_fmt_lexer* lexer, U8 ch)
{
  if (__str8_fmt_lexer_peek_char(lexer) == ch) {
    __str8_fmt_lexer_eat_char(lexer);
    return true;
  }
  return false;
}

B32 __str8_fmt_lexer_match_str(__Str8_fmt_lexer* lexer, Str8 str)
{
  B32 is_match = false;
  if (lexer->current_pos + str.count <= lexer->original_str.count)
  {
    Str8 test_sub_str = str8_substring(lexer->original_str, lexer->current_pos, lexer->current_pos + str.count);
    is_match = str8_match(test_sub_str, str, 0);
    if (is_match)
    {
      lexer->current_pos += str.count;
    }
  }
  return is_match;
}

__Str8_fmt_token __str8_fmt_lexer_make_token(__Str8_fmt_lexer* lexer, __Str8_fmt_token_kind kind)
{
  __Str8_fmt_token token = {};
  token.kind = kind;
  token.str = str8_substring(lexer->original_str, lexer->token_start_pos, lexer->current_pos);
  lexer->token_start_pos = lexer->current_pos;
  return token;
}

// - fmt buffer manipulations
U64 __str8_fmt_size_for_formatted_str_or_fill_buffer(const char* fmt, va_list* va_list, Data_buffer* out_opt_buffer)
{
  __Str8_fmt_lexer lexer = {};
  lexer.original_str = str8_manuall((U8*)fmt, strlen(fmt));

  U64 return_size = 0;
  U8* buffer_byte  = (out_opt_buffer ? out_opt_buffer->data : 0);
  U64 buffer_bytes_left = (out_opt_buffer ? out_opt_buffer->count : 0);
  
  for (;;)
  {
    __Str8_fmt_token token = __str8_fmt_lexer_eat_token(&lexer);
    if (token.kind == __Str8_fmt_token_kind__NONE) { break; }

    B32 stop_iterating = false;
    U64 bytes_for_token = 0;

    switch (token.kind)
    {
      default: {  } break;

      case __Str8_fmt_token_kind__U64: case __Str8_fmt_token_kind__B64: 
      case __Str8_fmt_token_kind__U: case __Str8_fmt_token_kind__B: 
      {
        U64 u64 = 0;
        switch(token.kind) {
          case __Str8_fmt_token_kind__U64: case __Str8_fmt_token_kind__B64: { u64 = (U64)va_arg(*va_list, U64); } break; 
          case __Str8_fmt_token_kind__U: case __Str8_fmt_token_kind__B: { u64 = (U64)va_arg(*va_list, U32); } break; 
        } 

        B32 is_bool = ((token.kind == __Str8_fmt_token_kind__B64) || 
                       (token.kind == __Str8_fmt_token_kind__B));
        
        U64 u64_str_size = (is_bool ? b64_as_str_size(u64) : u64_as_str8_size(u64));
        bytes_for_token = u64_str_size;

        if (out_opt_buffer)
        {
          // Damian: I know, i know, sorry for all these ifs here, this way there is just less code and 
          //         fmt is only needed once, so who cares i guess then.
          bytes_for_token = (is_bool ? __str8_fmt_put_b64_into_buffer : __str8_fmt_put_u64_into_buffer)(buffer_byte, buffer_bytes_left, u64);
          if (bytes_for_token != u64_str_size) {
            stop_iterating = true;
          }
        }
      } break;

      case __Str8_fmt_token_kind__S64:
      case __Str8_fmt_token_kind__S:
      {
        S64 s64 = 0;
        switch (token.kind) {
          case __Str8_fmt_token_kind__S64: { s64 = (S64)va_arg(*va_list, S64); } break;
          case __Str8_fmt_token_kind__S:   { s64 = (S64)va_arg(*va_list, S32); } break; 
        }  

        U64 s64_str_size = s64_as_str8_size(s64);
        bytes_for_token = s64_str_size;

        if (out_opt_buffer)
        {
          bytes_for_token = __str8_fmt_put_s64_into_buffer(buffer_byte, buffer_bytes_left, s64);
          if (bytes_for_token != s64_str_size) {
            stop_iterating = true;
          }
        }
      } break;

      case __Str8_fmt_token_kind__Pointer:
      {
        void* p = va_arg(*va_list, void*);
        U64 addr = (U64)p;

        U64 bytes_for_addr = u64_as_str8_size(addr); 
        bytes_for_token = bytes_for_addr;
        if (out_opt_buffer)
        {
          bytes_for_token = __str8_fmt_put_u64_into_buffer(buffer_byte, buffer_bytes_left, addr);
          if (bytes_for_token != bytes_for_addr) {
            stop_iterating = true;
          }
        }
      } break;

      case __Str8_fmt_token_kind__Str8:
      {
        Str8 str = va_arg(*va_list, Str8);
        bytes_for_token += str.count;
        if (out_opt_buffer)
        {
          bytes_for_token = __str8_fmt_put_str_into_buffer(buffer_byte, buffer_bytes_left, str);
          if (bytes_for_token != str.count) {
            stop_iterating = true;
          }
        }
      } break;

      case __Str8_fmt_token_kind__other:
      {
        bytes_for_token = token.str.count;
        if (out_opt_buffer)
        {
          bytes_for_token = __str8_fmt_put_str_into_buffer(buffer_byte, buffer_bytes_left, token.str);
          if (bytes_for_token != token.str.count) {
            stop_iterating = true;
          }
        }
      } break;
    } // switch onto the token kind

    if (stop_iterating) { break; }
    return_size += bytes_for_token;
    buffer_byte += bytes_for_token;
    buffer_bytes_left -= bytes_for_token;

  } // for loop over tokens
  return return_size;
}

U64 __str8_fmt_put_u64_into_buffer(U8* buffer, U64 buffer_size, U64 v)
{
  U64 bytes_written = 0; 
  U64 size_for_str = u64_as_str8_size(v);
  if (size_for_str <= buffer_size)
  {
    U64 divider = 1; 
    for (U64 i = size_for_str; i > 1 ; i -= 1)
    { 
      divider *= 10;
    }
    U64 buffer_pos = 0;
    for (U64 i = 0; i < size_for_str; i += 1)
    {
      U64 top_digit = v / divider;
      v -= top_digit * divider;
      divider /= 10;
      buffer[buffer_pos++] = (U8)top_digit + '0';
    }
    Assert(divider == 0);
    bytes_written = size_for_str;
  }
  return bytes_written;
}

U64 __str8_fmt_put_s64_into_buffer(U8* buffer, U64 buffer_size, S64 v)
{
  U64 size_for_str = s64_as_str8_size(v);
  if (size_for_str > buffer_size) { return 0; }

  U64 buffer_pos = 0;
  if (v < 0) 
  {
    v *= -1;
    buffer[buffer_pos++] = '-';
  }
  buffer_pos += __str8_fmt_put_u64_into_buffer(buffer + buffer_pos, buffer_size - buffer_pos, (U64)v);
  return buffer_pos;
}

U64 __str8_fmt_put_str_into_buffer(U8* buffer, U64 buffer_size, Str8 str)
{
  if (str.count > buffer_size) { return 0; }
  for (U64 i = 0; i < str.count; i += 1)
  {
    buffer[i] = str.data[i];
  }
  return str.count;
}

U64 __str8_fmt_put_b64_into_buffer(U8* buffer, U64 buffer_size, B64 b)
{
  Str8 str = (b ? Str8FromC("true") : Str8FromC("false"));
  U64 bytes_written = __str8_fmt_put_str_into_buffer(buffer, buffer_size, str);
  return bytes_written;
}


#endif

