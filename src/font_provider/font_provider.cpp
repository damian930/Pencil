#ifndef FONT_PROVIDER_CPP
#define FONT_PROVIDER_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

// #include "font_provider.h"

// For packing data into data atlases
#ifndef STB_RECT_PACK_IMPLEMENTATION 
#define STB_RECT_PACK_IMPLEMENTATION
#include "__third_party/stb/stb_rect_pack.h"
#endif

// For getting font data from ttf files
#ifndef STB_TRUETYPE_IMPLEMENTATION 
#define STB_TRUETYPE_IMPLEMENTATION
#include "__third_party/stb/stb_truetype.h"
#endif

// struct Font {

// };

#include "os/win32.h"
#include "os/win32.cpp"

// todo: Trying to just load a font like in raylib
void fp_test(Str8 ttf_file_path, F32 font_size, RangeU64 unicode_range_to_load)
{
  Scratch scratch = get_scratch(0, 0);
  
  // todo: Ask the user for the things to load from the font
  //       Load the things
  //       Pack the things into the atlas
  //       Store data for things packed to later access
  //       Load the atlas onto gpu
  //       Release all the stuff allocated by stb and you

  OS_File font_file = os_file_open(ttf_file_path, OS_File_access__visible_read);
  Handle(os_file_is_valid(font_file));

  Data_buffer font_data_buffer = data_buffer_make(scratch.arena, os_file_get_props(font_file).size);
  B32 read_succ = os_file_read(font_file, &font_data_buffer);
  Handle(read_succ);

  int font_index = stbtt_GetFontOffsetForIndex(font_data_buffer.data, 0);
  Handle(font_index);

  stbtt_fontinfo stb_font_info = {};
  int intit_ret = stbtt_InitFont(&stb_font_info, font_data_buffer.data, 0);
  Handle(intit_ret != -1);

  Data_buffer font_atlas_buffer = data_buffer_make(scratch.arena, 1024 * 1024);
  
  stbtt_pack_context stb_pack_context = {};
  int pack_begin_res = stbtt_PackBegin(&stb_pack_context, font_atlas_buffer.data, 1024, 1024, 0, 1, Null);
  Handle(pack_begin_res == 1);
  
  Assert(unicode_range_to_load.min <= s32_max);

  // This is what i need to get  
  stbtt_packedchar* packed_char_data_arr = ArenaPushArr(scratch.arena, stbtt_packedchar, range_u64_count(unicode_range_to_load));

  stbtt_pack_range stb_pack_range = {}; 
  stb_pack_range.font_size                        = font_size;
  stb_pack_range.first_unicode_codepoint_in_range = (int)unicode_range_to_load.min; 
  stb_pack_range.array_of_unicode_codepoints      = 0;       
  stb_pack_range.num_chars                        = (int)range_u64_count(unicode_range_to_load); // todo: This truncated u64 to int
  stb_pack_range.chardata_for_range               = packed_char_data_arr; 
  
  // todo: These return if fails, handle these
  stbtt_PackSetOversampling(&stb_pack_context, 1, 1); // note: 1,1 here means that no ovesampling is aplied
  stbtt_PackFontRanges(&stb_pack_context, font_data_buffer.data, 0, &stb_pack_range, 1);
  stbtt_PackEnd(&stb_pack_context);

  for EachIndex(packed_char_data_index, range_u64_count(unicode_range_to_load))
  {
    stbtt_packedchar* packed_char_data = packed_char_data_arr + packed_char_data_index;

    BP;
  }



  end_scratch(&scratch);
}







#endif