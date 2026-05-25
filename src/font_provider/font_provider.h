#ifndef FONT_PROVIDER_H
#define FONT_PROVIDER_H

#include "__third_party/stb/stb_rect_pack.h"
#include "__third_party/stb/stb_truetype.h"

#include "core/core_include.h"
#include "os/win32.h"
#include "render/render.h"

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
  // ID3D11RenderTargetView* atlas_texture;
  R_Target atlas_texture;

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

extern global FP_State* __fp_g_state;

// - State
FP_State* fp_get_state();
void fp_init();
void fp_release();

// - Stuff
FP_Font fp_load_font(Str8 ttf_file_path, F32 font_size, RangeU64 unicode_range_to_load);
FP_Codepoint_data fp_get_glyph_data(FP_Font font, U64 unicode_codepoint);
FP_Kerning_entry fp_get_kerning(FP_Font font, U64 unicode_codepoint_1, U64 unicode_codepoint_2);
V2F32 fp_measure_text(Str8 str, FP_Font font);
F32 fp_get_font_height(FP_Font font);

B32 __fp_try_to_generate_grey_scale_font_atlas_image(
  stbtt_packedchar* packed_char_data_arr,
  Data_buffer* font_atlas_buffer, U64 atlas_width_in_px, U64 atlas_height_in_px, 
  Data_buffer font_data_buffer, RangeU64 unicode_range_to_load, F32 font_size
);

#endif