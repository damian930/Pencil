#ifndef FONT_PROVIDER_CPP
#define FONT_PROVIDER_CPP

// todo: Do we need these here really ???
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

#include "core/core_include.h"
#include "core/core_include.cpp"

// todo: Does it need this ???
#include "os/win32.h"
#include "os/win32.cpp"

// todo: Move this into .h
#include "renderer/renderer.h"
#include "renderer/renderer.cpp"

// todo: Use this "stbtt_PackSetSkipMissingCodepoints"

struct FP_Codepoint_data {
  Rect rect_on_atlas; // The bounding box for the codepoint glyph in the texture font atlas. todo: Write for what this shoud be used.
  F32 bearing_x; // Positive
  F32 bearing_y; // Positive
  F32 advance;   
};

struct FP_Kerning_entry {
  U64 codepoint1;
  U64 codepoint2;
  F32 advance; // Might be negative or positive, so just add it to the normal advance for a codepoint
};

struct FP_Font {
  ID3D11RenderTargetView* atlas_texture;
  
  RangeU64 codepoint_range;
  FP_Codepoint_data* codepoints_data_arr;
  
  FP_Kerning_entry* kerning_entries;
  U64 kerning_entry_count;

  F32 size;   
  F32 ascent;  // Positive
  F32 descent; // Positive
};

struct FP_Font_node {
  FP_Font font;
  FP_Font_node* next;
  FP_Font_node* prev;
};

struct FP_State {
  Arena* state_arena;
  FP_Font_node* first_font;
  FP_Font_node* last_font;
  U64 font_count;
};

global FP_State* __fp_g_state = {};

FP_State* fp_get_state()
{
  return __fp_g_state;
}

void fp_init()
{
  Arena* arena = arena_alloc(Megabytes(64));
  __fp_g_state = ArenaPush(arena, FP_State);
  __fp_g_state->state_arena = arena;
}

void fp_release()
{
  arena_release(__fp_g_state->state_arena);
  __fp_g_state = 0; 
}

B32 __fp_try_to_generate_grey_scale_font_atlas_image(
  stbtt_packedchar* packed_char_data_arr,
  Data_buffer* font_atlas_buffer, U64 atlas_width_in_px, U64 atlas_height_in_px, 
  Data_buffer font_data_buffer, RangeU64 unicode_range_to_load, F32 font_size
) {
  Scratch scratch = get_scratch(0, 0);
  
  Handle(unicode_range_to_load.min <= s32_max);
  
  stbtt_pack_context stb_pack_context = {};
  int pack_begin_res = stbtt_PackBegin(&stb_pack_context, font_atlas_buffer->data, (int)atlas_width_in_px * 1, (int)atlas_height_in_px * 1, 0, 1, Null); // todo: I dont like this U64 to int truncation here at all
  Handle(pack_begin_res == 1);
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
FP_Font fp_load_font(Str8 ttf_file_path, F32 font_size, RangeU64 unicode_range_to_load)
{
  // note:
  // I spent so much time on this whole thing here, i wanted to handle cases now and dont just put it inside Handle,
  // but i didnt like nested ifs and i tried to find a way to do it better, i might have did a bad thing
  // spedning so much time like this on this, so for now i will just leave this here like that, cause 
  // i cant do it yet.

  // todo: Handle invalid parameters in here if

  FP_State* fp_state = fp_get_state();
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

  int kerning_table_length = stbtt_GetKerningTableLength(&stb_font_info);

  // Allocating a new font
  FP_Font* result_font = {};
  {
    FP_Font_node* new_font_node = ArenaPush(fp_state->state_arena, FP_Font_node);
    DllPushBack_Name(fp_state, new_font_node, first_font, last_font, next, prev);
    fp_state->font_count += 1;
    
    result_font = &new_font_node->font; 
    result_font->codepoints_data_arr = ArenaPushArr(fp_state->state_arena, FP_Codepoint_data, range_u64_count(unicode_range_to_load));;
  
    result_font->kerning_entries     = ArenaPushArr(fp_state->state_arena, FP_Kerning_entry, kerning_table_length);
    result_font->kerning_entry_count = (U64)kerning_table_length;
  }
    
  stbtt_packedchar* packed_char_data_arr = ArenaPushArr(scratch.arena, stbtt_packedchar, range_u64_count(unicode_range_to_load));

  U64 font_atlas_width  = 256;
  U64 font_atlas_height = 256;
  Data_buffer font_atlas_buffer = {};
  for (U64 attempt = 0; attempt < 3; attempt += 1)
  {
    U64 arena_pos_before_alloc = arena_get_pos(scratch.arena);

    font_atlas_buffer = data_buffer_make(scratch.arena, font_atlas_width * font_atlas_height);
    
    B32 succ = __fp_try_to_generate_grey_scale_font_atlas_image(packed_char_data_arr, &font_atlas_buffer, font_atlas_width, font_atlas_height, font_data_buffer, unicode_range_to_load, font_size); 

    if (succ) { break; }
    else {
      arena_pop_to_pos(scratch.arena, arena_pos_before_alloc);
      font_atlas_width *= 2;
      font_atlas_height *= 2;
      font_atlas_buffer = Data_buffer{};
    }
  }
  Handle(!IsMemZero(font_atlas_buffer)); // Not handling fail to create an atlas that might be too big or something like that

  F32 scale = stbtt_ScaleForPixelHeight(&stb_font_info, font_size);

  // Making an rgba texture for the font atlas
  U32* image_bytes = ArenaPushArr(scratch.arena, U32, font_atlas_width * font_atlas_height);
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

  Image rgba_font_atlas_image = {};
  rgba_font_atlas_image.data            = (U8*)image_bytes;
  rgba_font_atlas_image.width_in_px     = font_atlas_width;
  rgba_font_atlas_image.height_in_px    = font_atlas_height;
  rgba_font_atlas_image.bytes_per_pixel = 4;


  ID3D11RenderTargetView* atlas_texture = 0;
  {
    atlas_texture = r_load_texture_from_image(rgba_font_atlas_image);
    Handle(atlas_texture != 0);
  }

  // Setting font data
  {
    result_font->size            = font_size;
    result_font->codepoint_range = unicode_range_to_load;
    result_font->atlas_texture   = atlas_texture;

    for (U64 codepoint_index = 0; codepoint_index < range_u64_count(unicode_range_to_load); codepoint_index += 1)
    {
      // todo: Test if xoff,yoff,xadvance from stbtt_packedchar are the same as from the stbtt functions that retrive baseline offsets and x advance for chars
      U64 codepoint                     = unicode_range_to_load.min + codepoint_index;
      FP_Codepoint_data* codepoint_data = result_font->codepoints_data_arr + codepoint_index;
      stbtt_packedchar* packed_data     = packed_char_data_arr + codepoint_index;

      {
        int advance   = 0;
        int bearing_x = 0;
        stbtt_GetCodepointHMetrics(&stb_font_info, codepoint, &advance, &bearing_x);
        codepoint_data->bearing_x = f32_floor((F32)bearing_x * scale);
        codepoint_data->advance           = f32_floor((F32)advance * scale);
      }

      {
        int ascent = 0; int descent = 0; int line_gap = 0;
        stbtt_GetFontVMetrics(&stb_font_info, &ascent, &descent, &line_gap);
        result_font->ascent  = f32_floor((F32)ascent * scale);
        result_font->descent = f32_floor((F32)descent * scale * -1.0f);
        // F32 fline_gap = f32_floor((F32)line_gap * scale);
      }

      Rect glyph_atlas_rect = rect_make((F32)packed_data->x0, (F32)packed_data->y0, (F32)(packed_data->x1 - packed_data->x0), (F32)(packed_data->y1 - packed_data->y0)); 
      codepoint_data->rect_on_atlas = glyph_atlas_rect;
      // if (glyph_atlas_rect.width > result_font->max_width) { result_font->max_width = glyph_atlas_rect.width; }
      // if (glyph_atlas_rect.height > result_font->max_height) { result_font->max_height = glyph_atlas_rect.height; }
    
      {
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        stbtt_GetCodepointBitmapBox(&stb_font_info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
        F32 fx0 = f32_floor((F32)x0);
        F32 fy0 = f32_floor((F32)y0);
        F32 fx1 = f32_floor((F32)x1);
        F32 fy1 = f32_floor((F32)y1);
        
        // if (codepoint == 'C') { BP; }

        codepoint_data->bearing_y = -1.0f * fy0;
        // InvariantCheck(codepoint_data->bearing_x == fx0);
        InvariantCheck(glyph_atlas_rect.width == fx1 - fx0);
        InvariantCheck(glyph_atlas_rect.height == fy1 - fy0);
      }
    }
  }

  InvariantCheck(result_font->kerning_entry_count == kerning_table_length);
  stbtt_kerningentry* stb_kerning_arr = ArenaPushArr(scratch.arena, stbtt_kerningentry, kerning_table_length);

  int kernings_written = stbtt_GetKerningTable(&stb_font_info, stb_kerning_arr, kerning_table_length);
  InvariantCheck(kernings_written == kerning_table_length);

  for (S32 i = 0; i < kerning_table_length; i += 1)
  {
    stbtt_kerningentry* stb_entry = stb_kerning_arr + i;
    FP_Kerning_entry* fp_entry    = result_font->kerning_entries + i;
    
    fp_entry->codepoint1 = stb_entry->glyph1;
    fp_entry->codepoint2 = stb_entry->glyph2;
    fp_entry->advance    = f32_floor((F32)stb_entry->advance * scale);
  }

  end_scratch(&scratch);
  return *result_font;
}

// todo: Have font provider give a null char that is set in the font file when we request a char that is not a part of the font

FP_Codepoint_data fp_get_glyph_data(FP_Font font, U64 unicode_codepoint)
{
  if (!range_u64_within(font.codepoint_range, unicode_codepoint)) { return FP_Codepoint_data{}; }

  FP_Codepoint_data* glyph_data = font.codepoints_data_arr + (unicode_codepoint - font.codepoint_range.min);
  return *glyph_data;
}

FP_Kerning_entry fp_get_kerning(FP_Font font, U64 unicode_codepoint_1, U64 unicode_codepoint_2)
{
  FP_Kerning_entry result_entry = {};
  for (U64 i = 0; i < font.kerning_entry_count; i += 1)
  {
    FP_Kerning_entry* entry = font.kerning_entries + i;
    if (entry->codepoint1 == unicode_codepoint_1 && entry->codepoint2 == unicode_codepoint_2)
    {
      result_entry = *entry;
      break;
    }
  }
  return result_entry;
}






#endif