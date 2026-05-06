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

B32 __fp_try_to_generate_grey_scal_font_atlas_image(
  Data_buffer* font_atlas_buffer, U64 atlas_width_in_px, U64 atlas_height_in_px, 
  Data_buffer font_data_buffer, RangeU64 unicode_range_to_load, F32 font_size
) {
  Scratch scratch = get_scratch(0, 0);
  
  Handle(unicode_range_to_load.min <= s32_max);
  
  stbtt_pack_context stb_pack_context = {};
  int pack_begin_res = stbtt_PackBegin(&stb_pack_context, font_atlas_buffer->data, (int)atlas_width_in_px * 1, (int)atlas_height_in_px * 1, 0, 1, Null); // todo: I dont like this U64 to int truncation here at all
  Handle(pack_begin_res == 1);
  
  stbtt_packedchar* packed_char_data_arr = ArenaPushArr(scratch.arena, stbtt_packedchar, range_u64_count(unicode_range_to_load));
  stbtt_pack_range stb_pack_range = {}; 
  stb_pack_range.font_size                        = font_size;
  stb_pack_range.first_unicode_codepoint_in_range = (int)unicode_range_to_load.min; 
  stb_pack_range.array_of_unicode_codepoints      = 0;       
  stb_pack_range.num_chars                        = (int)range_u64_count(unicode_range_to_load); // todo: This truncated u64 to int
  stb_pack_range.chardata_for_range               = packed_char_data_arr; 
    
  B32 succ = true;
  stbtt_PackSetOversampling(&stb_pack_context, 1, 1); // note: 1,1 here means that no ovesampling is aplied
  int pack_succ = stbtt_PackFontRanges(&stb_pack_context, font_data_buffer.data, 0, &stb_pack_range, 1);
  if (pack_succ == 0) { succ = false; }
  else { stbtt_PackEnd(&stb_pack_context); }

  end_scratch(&scratch);
  return succ;
}

// todo: Trying to just load a font like in raylib
// note: Returs all zeroes out Image struct memory on fail
Image font_provider_create_cpu_side_font_atlas(Arena* image_arena, Str8 ttf_file_path, F32 font_size, RangeU64 unicode_range_to_load)
{
  // note:
  // I spent so much time on this whole thing here, i wanted to handle cases now and dont just put it inside Handle,
  // but i didnt like nested ifs and i tried to find a way to do it better, i might have did a bad thing
  // spedning so much time like this on this, so for now i will just leave this here like that, cause 
  // i cant do it yet.

  // todo: Handle invalid parameters in here if

  Scratch scratch = get_scratch(0, 0);
  
  OS_File font_file = os_file_open(ttf_file_path, OS_File_access__visible_read);
  Handle(os_file_is_valid(font_file));

  Data_buffer font_data_buffer = data_buffer_make(scratch.arena, os_file_get_props(font_file).size);
  B32 read_succ = os_file_read(font_file, &font_data_buffer);
  Handle(read_succ);

  int font_index = stbtt_GetFontOffsetForIndex(font_data_buffer.data, 0);
  if (font_index > 0) { NotImplemented(); }
  Handle(font_index != -1);
  
  stbtt_fontinfo stb_font_info = {}; // note: This does not have to be freed
  int stb_font_init = stbtt_InitFont(&stb_font_info, font_data_buffer.data, 0);
  Handle(stb_font_init != -1);
          
  // todo:
  U64 font_atlas_width  = 256;
  U64 font_atlas_height = 256;
  Data_buffer font_atlas_buffer = {};
  for (U64 attempt = 0; attempt < 2; attempt += 1)
  {
    U64 arena_pos_before_alloc = arena_get_pos(scratch.arena);

    font_atlas_buffer = data_buffer_make(scratch.arena, font_atlas_width * font_atlas_height);
    B32 succ = __fp_try_to_generate_grey_scal_font_atlas_image(&font_atlas_buffer, font_atlas_width, font_atlas_height, font_data_buffer, unicode_range_to_load, font_size); 

    if (succ) { break; }
    else {
      arena_pop_to_pos(scratch.arena, arena_pos_before_alloc);
      font_atlas_width *= 2;
      font_atlas_height *= 2;
      font_atlas_buffer = Data_buffer{};
    }
  }
  Handle(!IsMemZero(font_atlas_buffer)); // Not handling fail to create an atlas that might be too big or something like that
  
  U32* image_bytes = ArenaPushArr(image_arena, U32, font_atlas_width * font_atlas_height);
  for (U64 y_byte_index = 0; y_byte_index < font_atlas_height; y_byte_index += 1)
  {
    for (U64 x_byte_index = 0; x_byte_index < font_atlas_width; x_byte_index += 1)
    {
      U8 intensity_pixel_value = font_atlas_buffer.data[y_byte_index * font_atlas_width + x_byte_index];
      U32* image_pixel         = image_bytes + (y_byte_index * font_atlas_width + x_byte_index);

      // todo: I am not sure if its supposed to be intensity,intensity,intensity,255 or 255,255,255,intensity 
      ((U8*)(image_pixel))[0] = intensity_pixel_value;
      ((U8*)(image_pixel))[1] = intensity_pixel_value;
      ((U8*)(image_pixel))[2] = intensity_pixel_value;
      ((U8*)(image_pixel))[3] = 255;
    }
  }

  Image result_image = {};
  result_image.data            = (U8*)image_bytes;
  result_image.width_in_px     = font_atlas_width;
  result_image.height_in_px    = font_atlas_height;
  result_image.bytes_per_pixel = 4;

  end_scratch(&scratch);
  return result_image;
}







#endif