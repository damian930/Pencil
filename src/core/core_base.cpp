#ifndef CORE_BASE_CPP
#define CORE_BASE_CPP

#include "core_base.h"

F32 abs_f32(F32 x) { F32 result = x; if (result < 0.0f) { result *= -1.0f; } return result; }
F64 abs_f64(F64 x) { F64 result = x; if (result < 0.0)  { result *= -1.0;  } return result; }
S8  abs_s8 (S8  x) { S8  result = x; if (result < 0) { result *= -1; } return result; }
S16 abs_s16(S16 x) { S16 result = x; if (result < 0) { result *= -1; } return result; }
S32 abs_s32(S32 x) { S32 result = x; if (result < 0) { result *= -1; } return result; }
S64 abs_s64(S64 x) { S64 result = x; if (result < 0) { result *= -1; } return result; }

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

F32 lerp_f32(F32 v0, F32 v1, F32 t) { F32 result = v0 + t * (v1 - v0); return result; }
F64 lerp_f64(F64 v0, F64 v1, F64 t) { F64 result = v0 + t * (v1 - v0); return result; }

U64 u64_from_2_u32(U32 high_order_word, U32 low_order_word)
{
	U64 result = ((((U64)high_order_word) << 32) | low_order_word);
	return result;
}

Time time_from_readable_time(Readable_time* r_time)
{
	Time time = {};
	U64 mult = 1;
	time += mult * r_time->milisecond;
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
	r_time.milisecond = time % 1000; time /= 1000;
	r_time.second 	  = time % 60; time /= 60;
	r_time.minute 	  = time % 60; time /= 60;
	r_time.hour   	  = time % 24; time /= 24;
	r_time.day    	  = time % 31; time /= 31;
	r_time.month  	  = (Month)(time % 12); time /= 12;
	r_time.year   	  = time;
	return r_time;
}

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

RangeS64 range_s64_make(S64 start, S64 end)
{
	RangeS64 r = {};
	r.min = start;
	r.max = end;
	return r;
}

U64 range_s64_count(RangeS64 range)
{
	U64 count = range.max - range.min;
	return count;
}

RangeU64 range_u64_make(U64 min, U64 max)
{
	RangeU64 range = {};
	range.min = min;
	range.max = max;
	return range;
}

U64 range_u64_count(RangeU64 range)
{
	U64 count = range.max - range.min;
	return count;
}

B32 range_u64_within(RangeU64 range, U64 v)
{
	B32 result = (range.min <= v && v < range.max);
	return result;
}

B32 range_u64_is_valid(RangeU64 range)
{
	B32 result = (range.min <= range.max);
	return result;
}

Vec2_F32 rect_dims(Rect rect)
{
	Vec2_F32 dims = {};
	dims.x = rect.width;
	dims.y = rect.height;
	return dims;
}

B32 is_point_inside_rect(F32 x, F32 y, Rect r)
{
	B32 is_inside = (
		r.x <= x && x < r.x + r.width  &&
		r.y <= y && y < r.y + r.height
	);
	return is_inside;	
}

B32 is_point_inside_rectV(Vec2_F32 v, Rect r)
{
	B32 is_inside = (
		r.x <= v.x && v.x < r.x + r.width  &&
		r.y <= v.y && v.y < r.y + r.height
	);
	return is_inside;	
}

B32 is_point_inside_line(V2 point, V2 line_start, V2 line_end)
{
	// This formula uses cross product. I dont understand why this works, but this is the formula
	F32 equation_left_side  = (line_end.y - line_start.y) * (point.x - line_start.x);
	F32 equation_right_side = (point.y - line_start.y) * (line_end.x - line_start.x);
	F32 diff = abs_f32(equation_left_side) - abs_f32(equation_right_side);
	B32 is_inside = (abs_f32(diff) < 0.01);
	return is_inside;
}


#endif




