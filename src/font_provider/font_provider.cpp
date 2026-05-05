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
// note: Returs all zeroes out Image struct memory on fail
Image font_provider_create_cpu_side_font_atlas(Arena* image_arena, Str8 ttf_file_path, F32 font_size, RangeU64 unicode_range_to_load)
{
  // todo: Handle invalid parameters in here if

  Scratch scratch = get_scratch(0, 0);
  Image result_image = {}; 

  OS_FileOpenClose(font_file, ttf_file_path, OS_File_access__visible_read)
  {
    if (os_file_is_valid(font_file))
    {
      Data_buffer font_data_buffer = data_buffer_make(scratch.arena, os_file_get_props(font_file).size);
      if (os_file_read(font_file, &font_data_buffer))
      {
        int font_index = stbtt_GetFontOffsetForIndex(font_data_buffer.data, 0);
        if (font_index > 0) { NotImplemented(); }
        if (font_index != -1) 
        {
          stbtt_fontinfo stb_font_info = {}; // note: This does not have to be freed
          if (stbtt_InitFont(&stb_font_info, font_data_buffer.data, 0))
          {
            U64 font_atlas_width_in_bytes  = 50;// 256;
            U64 font_atlas_height_in_bytes = 50;// 256;
            Data_buffer font_atlas_buffer  = data_buffer_make(scratch.arena, font_atlas_width_in_bytes * font_atlas_height_in_bytes);;
            
            Handle(unicode_range_to_load.min <= s32_max);

            stbtt_pack_context stb_pack_context = {};
            for (U64 attempt = 0; attempt < 2; attempt += 1)
            {
              B32 created_an_atlas = false;
              if (stbtt_PackBegin(&stb_pack_context, font_atlas_buffer.data, font_atlas_width_in_bytes, font_atlas_height_in_bytes, 0, 1, Null))
              {
                U64 scratch_pos_attempt_start = arena_get_bytes_used(scratch.arena);
                stbtt_packedchar* packed_char_data_arr = ArenaPushArr(scratch.arena, stbtt_packedchar, range_u64_count(unicode_range_to_load));
  
                stbtt_pack_range stb_pack_range = {}; 
                stb_pack_range.font_size                        = font_size;
                stb_pack_range.first_unicode_codepoint_in_range = (int)unicode_range_to_load.min; 
                stb_pack_range.array_of_unicode_codepoints      = 0;       
                stb_pack_range.num_chars                        = (int)range_u64_count(unicode_range_to_load); // todo: This truncated u64 to int
                stb_pack_range.chardata_for_range               = packed_char_data_arr; 
                
                stbtt_PackSetOversampling(&stb_pack_context, 1, 1); // note: 1,1 here means that no ovesampling is aplied
                int pack_succ = stbtt_PackFontRanges(&stb_pack_context, font_data_buffer.data, 0, &stb_pack_range, 1);
                if (!pack_succ) {
                  arena_pop_to(scratch.arena, scratch_pos_attempt_start);
                }
                if (pack_succ)
                {
                  stbtt_PackEnd(&stb_pack_context);
                  created_an_atlas = true;
  
                  U32* image_bytes = ArenaPushArr(image_arena, U32, font_atlas_width_in_bytes * font_atlas_height_in_bytes);
                  for (U64 y_byte_index = 0; y_byte_index < font_atlas_height_in_bytes; y_byte_index += 1)
                  {
                    for (U64 x_byte_index = 0; x_byte_index < font_atlas_width_in_bytes; x_byte_index += 1)
                    {
                      U8 intensity_pixel_value = font_atlas_buffer.data[y_byte_index * font_atlas_width_in_bytes + x_byte_index];
                      U32* image_pixel         = image_bytes + (y_byte_index * font_atlas_width_in_bytes + x_byte_index);
  
                      ((U8*)(image_pixel))[0] = 255;
                      ((U8*)(image_pixel))[1] = 255;
                      ((U8*)(image_pixel))[2] = 255;
                      ((U8*)(image_pixel))[3] = intensity_pixel_value;
                    }
                  }
                  result_image.data            = (U8*)image_bytes;
                  result_image.width_in_px     = font_atlas_width_in_bytes;
                  result_image.height_in_px    = font_atlas_height_in_bytes;
                  result_image.bytes_per_pixel = 4;
                }
              }

              if (created_an_atlas) { break; }
              font_atlas_width_in_bytes *= 2;
              font_atlas_height_in_bytes *= 2;
            }
          }
        }
      }
    }
  }

  end_scratch(&scratch);
  return result_image;
}







#endif