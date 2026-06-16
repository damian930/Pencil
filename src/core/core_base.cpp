#ifndef CORE_BASE_CPP
#define CORE_BASE_CPP

#include "core_base.h"

///////////////////////////////////////////////////////////
// - Axis2 
//
Axis2 axis2_other(Axis2 axis) { return (axis == Axis2__x ? Axis2__y : Axis2__x); }

///////////////////////////////////////////////////////////
// - V2F32 
//
V2F32 v2f32       (F32 x, F32 y)       { return { x, y }; }
B32   v2f32_match (V2F32 v1, V2F32 v2) { return (v1.x == v2.x && v1.y == v2.y); }
F32   v2f32_len_sq(V2F32 v)            { return (v.x * v.x) + (v.y * v.y); }
F32   v2f32_len   (V2F32 v)            { return sqrtf(v2f32_len_sq(v)); }
V2F32 v2f32_sub   (V2F32 v1, V2F32 v2) { return v2f32(v1.x - v2.x, v1.y - v2.y); }

///////////////////////////////////////////////////////////
// - V2U16 
//
V2U16 v2u16      (U16 x, U16 y)       { return { x, y }; }
B32   v2u16_match(V2U16 v1, V2U16 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2U32 
//
V2U32 v2u32      (U32 x, U32 y)       { return { x, y }; }
B32   v2u32_match(V2U32 v1, V2U32 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2U64 
//
V2U64 v2u64      (U64 x, U64 y)       { return { x, y }; }
B32   v2u64_match(V2U64 v1, V2U64 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2S8 
//
V2S8  v2s8      (S8 x, S8 y)          { return { x, y }; }
B32   v2s8_match(V2S8 v1, V2S8 v2)    { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2S16 
V2S16 v2s16      (S16 x, S16 y)       { return { x, y }; }
B32   v2s16_match(V2S16 v1, V2S16 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2S32 
//
V2S32 v2s32      (S32 x, S32 y)       { return { x, y }; }
B32   v2s32_match(V2S32 v1, V2S32 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V2S64 
//
V2S64 v2s64      (S64 x, S64 y)       { return { x, y }; }
B32   v2s64_match(V2S64 v1, V2S64 v2) { return (v1.x == v2.x && v1.y == v2.y); }

///////////////////////////////////////////////////////////
// - V3F32 
//
V3F32 v3f32(F32 x, F32 y, F32 z) { V3F32 v = { x, y, z }; return v; }

///////////////////////////////////////////////////////////
// - V4F32 
//
V4F32 v4f32      (F32 x, F32 y, F32 z, F32 w) { V4F32 v = { x, y, z, w }; return v; }
B32   v4f32_match(V4F32 v1, V4F32 v2)         { return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w); }
V4F32 v4f32_all  (F32 x)                      { return v4f32(x, x, x, x); }

///////////////////////////////////////////////////////////
// - V4U8 
//
V4U8 v4u8(U8 x, U8 y, U8 z, U8 w) { return { x, y, z, w }; }

///////////////////////////////////////////////////////////
// - RangeF32 
//
RangeF32 rangef32               (F32 min, F32 max)               { RangeF32 r = {}; r.min = min; r.max = max; return r; }
F32      rangef32_length        (RangeF32 range)                 { return range.max - range.min; }
B32      rangef32_within        (RangeF32 range, F32 v)          { return (range.min <= v && v < range.max); }
B32      rangef32_is_valid      (RangeF32 range)                 { return (range.min <= range.max); }
B32      rangef32_contains_range(RangeF32 range, RangeF32 other) { return (range.min <= other.min && other.max <= range.max); }

///////////////////////////////////////////////////////////
// - RangeS64 
//
RangeS64 ranges64       (S64 start, S64 end) { RangeS64 r = {}; r.min = start; r.max = end; return r; }
U64      range_s64_count(RangeS64 range)     { return range.max - range.min; }

///////////////////////////////////////////////////////////
// - RangeU64 
//
RangeU64 rangeu64          (U64 min, U64 max)      { RangeU64 r = {}; r.min = min; r.max = max; return r; }
U64      range_u64_count   (RangeU64 range)        { return range.max - range.min; }
B32      range_u64_within  (RangeU64 range, U64 v) { return (range.min <= v && v < range.max); }
B32      range_u64_is_valid(RangeU64 range)        { return (range.min <= range.max); }

///////////////////////////////////////////////////////////
// - Rect 
//
Rect  rect_make            (F32 x, F32 y, F32 width, F32 height) { Rect r = {}; r.x = x; r.y = y; r.width = width; r.height = height; return r; }
Rect  rect_make_v          (V2F32 pos, V2F32 dims)               { return rect_make(pos.x, pos.y, dims.x, dims.y); }
Rect  rect_from_center     (V2F32 center, V2F32 dims)            { Rect r = {}; r.x = center.x - (dims.x / 2.0f); r.y = center.y - (dims.y / 2.0f); r.width = dims.x; r.height = dims.y; return r; }
Rect  rect_from_range_v2f32(RangeV2F32 range)                    { return rect_make(range.min.x, range.min.y, range.max.x - range.min.x, range.max.y - range.min.y); }
V2F32 rect_get_center      (Rect rect)                           { return v2f32(rect.x + (rect.width / 2.0f), rect.y + (rect.height / 2.0f)); }
B32   rect_match           (Rect r1, Rect r2)                    { return (r1.x == r2.x && r1.y == r2.y && r1.width == r2.width && r1.height == r2.height); }
B32   is_point_inside_rect (F32 x, F32 y, Rect r)                { return (r.x <= x && x < r.x + r.width && r.y <= y && y < r.y + r.height); }
B32   is_point_inside_rectV(V2F32 v, Rect r)                     { return (r.x <= v.x && v.x < r.x + r.width && r.y <= v.y && v.y < r.y + r.height); }

Rect rect_padded(Rect rect, F32 padd)
{
	Rect result  = rect;
	result.x    -= padd;
	result.y    -= padd;
	result.width  += 2 * padd;
	result.height += 2 * padd;
	if (result.width  < 0.0f) { result.width  = 0.0f; }
	if (result.height < 0.0f) { result.height = 0.0f; }
	return result;
}

///////////////////////////////////////////////////////////
// - RangeV2F32
//
RangeV2F32 range_v2f32          (V2F32 min, V2F32 max)               { RangeV2F32 r = {}; r.min = min; r.max = max; return r; }
RangeV2F32 range_v2f32_from_rect(Rect rect)                          { return range_v2f32(v2f32(rect.x, rect.y), v2f32(rect.x + rect.width, rect.y + rect.height)); }
V2F32      range_v2f32_x0y0     (RangeV2F32 r)                       { return v2f32(r.min.x, r.min.y); }
V2F32      range_v2f32_x1y0     (RangeV2F32 r)                       { return v2f32(r.max.x, r.min.y); }
V2F32      range_v2f32_x0y1     (RangeV2F32 r)                       { return v2f32(r.min.x, r.max.y); }
V2F32      range_v2f32_x1y1     (RangeV2F32 r)                       { return v2f32(r.max.x, r.max.y); }
RangeF32   range_v2f32_x0x1     (RangeV2F32 r)                       { return rangef32(r.min.x, r.max.x); }
RangeF32   range_v2f32_y0y1     (RangeV2F32 r)                       { return rangef32(r.min.y, r.max.y); }
RangeV2F32 range_v2f32_as_bb    (V2F32 p1, V2F32 p2)                 { return range_v2f32(v2f32(Min(p1.x, p2.x), Min(p1.y, p2.y)), v2f32(Max(p1.x, p2.x), Max(p1.y, p2.y))); }
B32        range_v2f32_match    (RangeV2F32 range, RangeV2F32 other) { return (v2f32_match(range.min, other.min) && v2f32_match(range.max, other.max)); }
B32        range_v2f32_within   (RangeV2F32 range, V2F32 vec)        { return (range.min.x <= vec.x && vec.x < range.max.x && range.min.y <= vec.y && vec.y < range.max.y); }

V2F32 range_v2f32_dims(RangeV2F32 range)
{
	V2F32 dims = {};
	dims.x = range.max.x - range.min.x;
	dims.y = range.max.y - range.min.y;
	Assert(dims.x >= 0.0f && dims.y >= 0.0f);
	return dims;
}

RangeV2F32 intersect_range_v2f32_on_axis(RangeV2F32 range, RangeV2F32 other, Axis2 axis)
{
	range.min.v[axis] = Max(range.min.v[axis], other.min.v[axis]);
	range.max.v[axis] = Min(range.max.v[axis], other.max.v[axis]);
	return range;
}

RangeV2F32 intersect_range_v2f32(RangeV2F32 range, RangeV2F32 other)
{
	RangeV2F32 intermediate = intersect_range_v2f32_on_axis(range, other, Axis2__x);
	RangeV2F32 result       = intersect_range_v2f32_on_axis(intermediate, other, Axis2__y);
	return result;
}

///////////////////////////////////////////////////////////
// - Abs
//
F32 abs_f32(F32 x) { F32 result = x; if (result < 0.0f) { result *= -1.0f; } return result; }
F64 abs_f64(F64 x) { F64 result = x; if (result < 0.0)  { result *= -1.0;  } return result; }
S8  abs_s8 (S8  x) { S8  result = x; if (result < 0) { result *= -1; } return result; }
S16 abs_s16(S16 x) { S16 result = x; if (result < 0) { result *= -1; } return result; }
S32 abs_s32(S32 x) { S32 result = x; if (result < 0) { result *= -1; } return result; }
S64 abs_s64(S64 x) { S64 result = x; if (result < 0) { result *= -1; } return result; }

///////////////////////////////////////////////////////////
// - Clamp
//
F32 clamp_f32(F32 value, F32 min, F32 max) { F32 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
F64 clamp_f64(F64 value, F64 min, F64 max) { F64 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
S8  clamp_s8 (S8  value, S8  min, S8  max) { S8  result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
S16 clamp_s16(S16 value, S16 min, S16 max) { S16 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
S32 clamp_s32(S32 value, S32 min, S32 max) { S32 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
S64 clamp_s64(S64 value, S64 min, S64 max) { S64 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
U8  clamp_u8 (U8  value, U8  min, U8  max) { U8  result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
U16 clamp_u16(U16 value, U16 min, U16 max) { U16 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
U32 clamp_u32(U32 value, U32 min, U32 max) { U32 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }
U64 clamp_u64(U64 value, U64 min, U64 max) { U64 result = value; if (result < min) { result = min; } else if (result > max) { result = max; } return result; }

void clamp_f32_inplace(F32* value, F32 min, F32 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_f64_inplace(F64* value, F64 min, F64 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_s8_inplace (S8*  value, S8  min, S8  max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_s16_inplace(S16* value, S16 min, S16 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_s32_inplace(S32* value, S32 min, S32 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_s64_inplace(S64* value, S64 min, S64 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_u8_inplace (U8*  value, U8  min, U8  max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_u16_inplace(U16* value, U16 min, U16 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_u32_inplace(U32* value, U32 min, U32 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }
void clamp_u64_inplace(U64* value, U64 min, U64 max) { if (*value < min) { *value = min; } else if (*value > max) { *value = max; } }

///////////////////////////////////////////////////////////
// - Lerp
//
F32   lerp_f32  (F32 v0, F32 v1, F32 t)     { return v0 + t * (v1 - v0); }
F64   lerp_f64  (F64 v0, F64 v1, F64 t)     { return v0 + t * (v1 - v0); }
V2F32 lerp_v2f32(V2F32 v0, V2F32 v1, F32 t) { return v2f32(lerp_f32(v0.x, v1.x, t), lerp_f32(v0.y, v1.y, t)); }
V3F32 lerp_v3f32(V3F32 v0, V3F32 v1, F32 t) { return v3f32(lerp_f32(v0.x, v1.x, t), lerp_f32(v0.y, v1.y, t), lerp_f32(v0.z, v1.z, t)); }
V4F32 lerp_v4f32(V4F32 v0, V4F32 v1, F32 t) { return v4f32(lerp_f32(v0.x, v1.x, t), lerp_f32(v0.y, v1.y, t), lerp_f32(v0.z, v1.z, t), lerp_f32(v0.w, v1.w, t)); }

F32 reverse_lerp_f32(F32 min, F32 max, F32 value) { return (value - min) / (max - min); }

///////////////////////////////////////////////////////////
// - F32
//
F32 f32_inf()
{
	union { U32 u32; F32 f32; } u;
	u.u32 = f32_exponent;
	return u.f32;
}

F32 f32_neg_inf()
{
	union { U32 u32; F32 f32; } u;
	u.u32 = f32_sign|f32_exponent;
	return u.f32;
}

F32 f32_nan()
{
	union { U32 u32; F32 f32; } u;
	u.u32 = f32_exponent;
	u.f32 = ~0;
	return (F32)u.f32;
}

B32 f32_is_nan(F32 f)
{
	union { U32 u32; F32 f32; } u;
	u.f32 = f;
	B32 result = false;
	if ((u.u32 & f32_exponent) == f32_exponent
			&&
			(u.u32 & f32_mantisa) > 0
	) {
		result = true;
	} 
	return result;
}

///////////////////////////////////////////////////////////
// - Colors
//
V4U8 transparent_u() { return v4u8(0, 0, 0, 0);         }
V4U8 black_u()       { return v4u8(0, 0, 0, 255);       }
V4U8 white_u()       { return v4u8(255, 255, 255, 255); }
V4U8 red_u()         { return v4u8(255, 0, 0, 255);     }
V4U8 green_u()       { return v4u8(0, 255, 0, 255);     }
V4U8 blue_u()        { return v4u8(0, 0, 255, 255);     }
V4U8 yellow_u()      { return v4u8(255, 255, 0, 255);   }
V4U8 pink_u()        { return v4u8(255, 0, 255, 255);   }
V4U8 teal_u()        { return v4u8(0, 128, 128, 255);   }
V4U8 orange_u()      { return v4u8(252, 102, 0, 255);   }
V4U8 taupe_u()       { return v4u8(146, 124, 102, 255); }
V4U8 magenta_u()     { return v4u8(253, 61, 181, 255);  }
V4U8 nice_green_u()  { return v4u8(120, 171, 128, 255); }
V4U8 nice_blue_u()   { return v4u8(97, 175, 239, 255 ); }
  
V4U8 change_alpha_u(V4U8 color, U8 new_a) { color.w = new_a; return color; }

// TODO: These shoud be macros since then you wont have to step into them in the debugger
V4F32 transparent() { return _U_COLOR_TO_F_COLOR(transparent_u()); } 
V4F32 black()       { return _U_COLOR_TO_F_COLOR(black_u());       } 
V4F32 white()       { return _U_COLOR_TO_F_COLOR(white_u());       } 
V4F32 red()         { return _U_COLOR_TO_F_COLOR(red_u());         }
V4F32 green()       { return _U_COLOR_TO_F_COLOR(green_u());       }
V4F32 blue()        { return _U_COLOR_TO_F_COLOR(blue_u());        }
V4F32 yellow()      { return _U_COLOR_TO_F_COLOR(yellow_u());      } 
V4F32 pink()        { return _U_COLOR_TO_F_COLOR(pink_u());        }  
V4F32 teal()        { return _U_COLOR_TO_F_COLOR(teal_u());        }  
V4F32 orange()      { return _U_COLOR_TO_F_COLOR(orange_u());      }  
V4F32 taupe()       { return _U_COLOR_TO_F_COLOR(taupe_u());       }  
V4F32 magenta()     { return _U_COLOR_TO_F_COLOR(magenta_u());     }  
V4F32 nice_green()  { return _U_COLOR_TO_F_COLOR(nice_green_u());  }  
V4F32 nice_blue()   { return _U_COLOR_TO_F_COLOR(nice_blue_u());   }

V4F32 color_change_alpha(V4F32 color, F32 new_a) { color.a = new_a; return color; }

V4F32 color_light_up(V4F32 color, F32 how_much_lighter) 
{
	color.r += how_much_lighter;
	color.g += how_much_lighter; 
	color.b += how_much_lighter;
	clamp_f32_inplace(&color.r, 0.0f, 1.0f);	 
	clamp_f32_inplace(&color.g, 0.0f, 1.0f);	 
	clamp_f32_inplace(&color.b, 0.0f, 1.0f);	 
	return color;
}

V4F32 rgba_from_rgb(V3F32 rgb, F32 a) { return v4f32(rgb.r, rgb.g, rgb.b, a); }
V3F32 rgb_from_rgba(V4F32 rgba)       { return v3f32(rgba.r, rgba.g, rgba.b); }

V3F32 hsv_from_rgb(V3F32 rgb) // Claude did this
{
	V3F32 hsv = v3f32(0.0f, 0.0f, 0.0f);

	F32 max = rgb.v[0];
	F32 min = rgb.v[0];
	U64 max_channel_index = 0;
	for (U64 i = 1; i < 3; i += 1)
	{
		if (rgb.v[i] > max) {
					max = rgb.v[i];
					max_channel_index = i;
			}

			if (rgb.v[i] < min) {
					min = rgb.v[i];
			}
	}
	F32 delta = max - min;
	
	hsv.value = max;
	if (max == 0.0f || delta == 0.0f) { return hsv; }
	hsv.saturation = delta / max;

	if      (max_channel_index == 0) { hsv.hue = (rgb.g - rgb.b) / delta;     }
	else if (max_channel_index == 1) { hsv.hue = (rgb.b - rgb.r) / delta + 2; }
	else if (max_channel_index == 2) { hsv.hue = (rgb.r - rgb.g) / delta + 4; }

	hsv.hue /= 6.0f;
	if (hsv.hue < 0.0f) { hsv.hue += 1.0f; }

	return hsv;
}

V3F32 rgb_from_hsv(V3F32 hsv) // This might be from the raddbg codebase
{
	F32 h = fmodf(hsv.hue * 360.f, 360.f);
  F32 s = hsv.saturation;
  F32 v = hsv.value;
  
  F32 c = v*s;
  F32 x = c*(1.f - abs_f32(fmodf(h/60.f, 2.f) - 1.f));
  F32 m = v - c;
  
  F32 r = 0;
  F32 g = 0;
  F32 b = 0;
  
  if ((h >= 0.f && h < 60.f) || (h >= 360.f && h < 420.f)){
    r = c;
    g = x;
    b = 0;
  }
  else if (h >= 60.f && h < 120.f){
    r = x;
    g = c;
    b = 0;
  }
  else if (h >= 120.f && h < 180.f){
    r = 0;
    g = c;
    b = x;
  }
  else if (h >= 180.f && h < 240.f){
    r = 0;
    g = x;
    b = c;
  }
  else if (h >= 240.f && h < 300.f){
    r = x;
    g = 0;
    b = c;
  }
  else if ((h >= 300.f && h <= 360.f) || (h >= -60.f && h <= 0.f)){
    r = c;
    g = 0;
    b = x;
  }
  
  V3F32 rgb = v3f32(r + m, g + m, b + m);
  return rgb;
}

V4F32 hsva_from_rgba(V4F32 rgba)
{
	V3F32 hsv = hsv_from_rgb(rgba.rgb);
	return v4f32(hsv.hue, hsv.saturation, hsv.value, rgba.a);
}

V4F32 rgba_from_hsva(V4F32 hsva)
{
	V3F32 rgb = rgb_from_hsv(hsva.hsv);
	return v4f32(rgb.r, rgb.g, rgb.b, hsva.a);
}

V4F32 rgba_from_hex(U32 hex)
{
	F32 a = (F32)(((U8*)&hex)[0]) / 255.0f;
	F32 b = (F32)(((U8*)&hex)[1]) / 255.0f;
	F32 g = (F32)(((U8*)&hex)[2]) / 255.0f;
	F32 r = (F32)(((U8*)&hex)[3]) / 255.0f;
	return v4f32(r, g, b, a);
}

V4F32 purify_rgb(V4F32 rgb)
{
	V4F32 hsv      = hsva_from_rgba(rgb);
	hsv.saturation = 1.0f;
	hsv.value      = 1.0f;
	V4F32 pure_rgb = rgba_from_hsva(hsv);
	return pure_rgb;
}

///////////////////////////////////////////////////////////
// - Memory
//
B32 __is_memory_zero(U8* p, U64 size)
{
	B32 is_zero = true;
	for (U64 i = 0; i < size; i += 1)
	{
		if (p[i] != 0) {
			is_zero = false;
			break;
		}
	}
	return is_zero;
}

///////////////////////////////////////////////////////////
// - Misc
//
Time time_from_readable_time(Readable_time* r_time)
{
	Time time = {};
	U64 mult = 1;
	time += mult * r_time->millisecond;
	mult *= 1000;	
	time += mult * r_time->second;
	mult *= 60;	
	time += mult * r_time->minute;
	mult *= 60;	
	time += mult * r_time->hour;
	mult *= 24;	
	time += mult * r_time->day;
	mult *= 31;	
	time += mult * r_time->month;
	mult *= 12;	
	time += mult * r_time->year;
	return time;
}

Readable_time readable_time_from_time(Time time)
{
	Readable_time r_time = {};
	r_time.millisecond = time % 1000; time /= 1000;
	r_time.second 	  = time % 60; time /= 60;
	r_time.minute 	  = time % 60; time /= 60;
	r_time.hour   	  = time % 24; time /= 24;
	r_time.day    	  = time % 31; time /= 31;
	r_time.month  	  = (Month)(time % 12); time /= 12;
	r_time.year   	  = time;
	return r_time;
}

#endif




