cbuffer cbuffer0 : register(b0) {
  float u_window_width;
  float u_window_height;
};

#define UV__00    0
#define UV__10    1
#define UV__01    2
#define UV__11    3
#define UV__COUNT 4 

struct VertexInput {
  float4 rect_color_00        : RECT_00_COLOR;  
  float4 rect_color_10        : RECT_10_COLOR;  
  float4 rect_color_01        : RECT_01_COLOR;  
  float4 rect_color_11        : RECT_11_COLOR;  
  
  float rect_origin_x         : RECT_ORIGIN_X; 
  float rect_origin_y         : RECT_ORIGIN_Y; 
  float rect_width            : RECT_WIDTH;
  float rect_height           : RECT_HEIGHT;
  
  float rect_corner_radius_00 : RECT_00_CORNER_RADIUS;
  float rect_corner_radius_10 : RECT_10_CORNER_RADIUS;
  float rect_corner_radius_01 : RECT_01_CORNER_RADIUS;
  float rect_corner_radius_11 : RECT_11_CORNER_RADIUS;
  
  float4 rect_border_color    : RECT_BORDER_COLOR;
  float rect_border_thickness : RECT_BORNER_THICKNESS;
  
  float softness              : SOFTNESS;

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  float4 vertex_color[UV__COUNT] : PER_VERTEX_COLOR;

  nointerpolation float2 rect_origin     : RECT_ORIGIN;
  nointerpolation float2 rect_dims       : RECT_DIMS;

  // todo: pass 4 differnt nointerpolation cor_rs
  nointerpolation float corner_radius  : CORNER_R;
  
  nointerpolation float4 border_color      : RECT_BORDER_COLOR;
  nointerpolation float border_thickness   : BORDER_THICH;
  
  nointerpolation float softness           : SOFTNESS;
  
  float4 pos : SV_POSITION;
};

float sdf_rounded_rect(float2 rect_origin, float2 rect_dims, float2 p, float r)
{
  float2 rect_max_point = rect_origin + rect_dims;
  float2 rect_half_dims = rect_dims / 2.0;
  float2 rect_mid       = (rect_max_point + rect_origin) / 2.0;
  
  float2 diff = abs(p - rect_mid) - rect_half_dims + float2(r, r); 
	return length(max(diff, 0.0)) + min(max(diff.x, diff.y), 0.0) - r;
}

bool is_point_inside_rect(float2 p, float2 origin, float2 dims)
{
  return (origin.x <= p.x && p.x < origin.x + dims.x && origin.y <= p.y && p.y < origin.y + dims.y);
}

PixelInput vs_main(VertexInput vertex_input) 
{
  float2 vp_dims     = float2(u_window_width, u_window_height);
  float2 rect_origin = float2(vertex_input.rect_origin_x, vertex_input.rect_origin_y);
  float2 rect_dims   = float2(vertex_input.rect_width, vertex_input.rect_height);

  float2 rect_vertex_coords[4] = {
    float2(0.0, 0.0), float2(1.0, 0.0),
    float2(0.0, 1.0), float2(1.0, 1.0),
  };
  
  float2 rect_vertex_in_px = rect_origin + (rect_vertex_coords[vertex_input.vertex_id] * rect_dims);

  // note: I hate that this takes 3 lines
  float2 rect_vertex_in_ndc = (rect_vertex_in_px / vp_dims) * 2.0;
  rect_vertex_in_ndc.x      = rect_vertex_in_ndc.x - 1.0; 
  rect_vertex_in_ndc.y      = 1.0 - rect_vertex_in_ndc.y;

  float rect_vertex_corner_r[4];
  rect_vertex_corner_r[0] = vertex_input.rect_corner_radius_00; 
  rect_vertex_corner_r[1] = vertex_input.rect_corner_radius_10;
  rect_vertex_corner_r[2] = vertex_input.rect_corner_radius_01; 
  rect_vertex_corner_r[3] = vertex_input.rect_corner_radius_11;
  for (uint i = 0; i < 4; i += 1) { rect_vertex_corner_r[i] = clamp(rect_vertex_corner_r[i], 0.0, 1.0); }

  PixelInput pixel_input;
  pixel_input.pos                  = float4(rect_vertex_in_ndc, 0, 1);
  pixel_input.rect_origin          = rect_origin;
  pixel_input.rect_dims            = rect_dims;
  pixel_input.corner_radius        = rect_vertex_corner_r[vertex_input.vertex_id];
  pixel_input.softness             = vertex_input.softness;
  pixel_input.border_thickness     = vertex_input.rect_border_thickness;
  pixel_input.border_color         = vertex_input.rect_border_color;
  pixel_input.vertex_color[UV__00] = vertex_input.rect_color_00;
  pixel_input.vertex_color[UV__10] = vertex_input.rect_color_10;
  pixel_input.vertex_color[UV__01] = vertex_input.rect_color_01;
  pixel_input.vertex_color[UV__11] = vertex_input.rect_color_11;

  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{
  float2 pos_px   = pixel_input.pos.xy;
  float2 pos_norm = (pos_px - pixel_input.rect_origin) / pixel_input.rect_dims; 

  float4 top_color    = lerp(pixel_input.vertex_color[UV__00], pixel_input.vertex_color[UV__10], pos_norm.x);
  float4 bottom_color = lerp(pixel_input.vertex_color[UV__01], pixel_input.vertex_color[UV__11], pos_norm.x);
  float4 final_color  = lerp(top_color, bottom_color, pos_norm.y);
  
  float softness = pixel_input.softness;

  float radius_in_px      = pixel_input.corner_radius * (min(pixel_input.rect_dims.x, pixel_input.rect_dims.y) / 2.0);
  float sdf_pixel_to_rect = sdf_rounded_rect(pixel_input.rect_origin, pixel_input.rect_dims, pos_px, radius_in_px);

  if (pixel_input.border_thickness > 0.0) {
    if (0.5 >= sdf_pixel_to_rect && sdf_pixel_to_rect >= -pixel_input.border_thickness) {
      final_color = pixel_input.border_color;
    } else {
      discard;
    }
  }

  if (sdf_pixel_to_rect > 0.0) { discard; }

  // note: no softness right now

  // if (softness != 0.0)
  // {
  //   // This does kind of soften the corners, but does remove the first pixel on the boundary,
  //   // i dont really like that.
  //   float smoothed = 1.0 - smoothstep(0.5 - softness, 0.5, sdf_pixel_to_rect);
    
  //   // This i randomly found out, but it makes sense. This does inside shadowing for a rect.
  //   // Maybe not shadowing, but something similar.
  //   // float smoothed = 1.0 - smoothstep(0.5 - softness, softness + 0.5, sdf_pixel_to_rect);
    
  //   final_color.a *= smoothed;
  // }

  /*
  float rect_outline_smoothing = 1.0f;
  {
    float2 inner_rect_origin = pixel_input.rect_origin + float2(softness, softness);
    float2 inner_rect_dims   = pixel_input.rect_dims - 2 * float2(softness, softness);
    float radius_in_px       = pixel_input.corner_radius * min(inner_rect_dims.x, inner_rect_dims.y) / 2.0;

    float rect_outline_sdf = sdf_rounded_rect(inner_rect_origin, inner_rect_dims, pos_px, radius_in_px);
    rect_outline_smoothing = 1 - smoothstep(0, 1.4*softness, rect_outline_sdf);
  }
  */

  /*
  float rect_inner_smoothing = 1.0f;
  if (pixel_input.border_thickness != 0.0)
  {
    final_color = pixel_input.border_color;

    float2 inner_rect_origin = pixel_input.rect_origin + float2(pixel_input.border_thickness + softness, pixel_input.border_thickness + softness);
    float2 inner_rect_dims   = pixel_input.rect_dims - 2 * float2(pixel_input.border_thickness + softness, pixel_input.border_thickness + softness);
    float radius_in_px       = pixel_input.corner_radius * min(inner_rect_dims.x, inner_rect_dims.y) / 2.0;

    float rect_outline_sdf = sdf_rounded_rect(inner_rect_origin, inner_rect_dims, pos_px, radius_in_px);
    if (rect_outline_sdf < -softness) { discard; }
    else {
      // rect_inner_smoothing = smoothstep(-softness, softness, rect_outline_sdf);
      rect_inner_smoothing = 1 - smoothstep(-2*softness, 0.0, -rect_outline_sdf);
    }
  } 
  */ 

  return final_color;

  // note: This is removed here, since rect_outline_smoothing makes the borders look white at some places for some reason.
  //       I dont have the will to fix the shader to be honest, i have already spent so much time on it. 
  //       Imma leave it for later
  // final_color.a *= rect_outline_smoothing;
  // final_color.a *= rect_inner_smoothing;
  // return final_color;
}