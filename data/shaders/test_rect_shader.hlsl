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
  
  float rect_border_thickness : RECT_BORNER_THICKNESS;

  float softness              : SOFTNESS;

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  float4 vertex_color[UV__COUNT] : PER_VERTEX_COLOR;

  float2 rect_origin     : RECT_ORIGIN;
  float2 rect_dims       : RECT_DIMS;

  float rect_border_thickness : RECT_BORDER_THICK;
  
  float corner_radius    : CORNER_R;
  float border_thickness : BORDER_THICH;
  float softness         : SOFTNESS;
  
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

float2 rect_vertex_coords[4] = {
  float2(0.0, 0.0), float2(1.0, 0.0),
  float2(0.0, 1.0), float2(1.0, 1.0),
};

PixelInput vs_main(VertexInput vertex_input) 
{
  float2 vp_dims     = float2(u_window_width, u_window_height);
  float2 rect_origin = float2(vertex_input.rect_origin_x, vertex_input.rect_origin_y);
  float2 rect_dims   = float2(vertex_input.rect_width, vertex_input.rect_height);

  float2 rect_vertex_coords[4] = {
    float2(0.0, 0.0), float2(1.0, 0.0),
    float2(0.0, 1.0), float2(1.0, 1.0),
  };
  
  // float2 bordered_rect_vertex_in_px = rect_vertex_in_px + ((rect_vertex_coords[vertex_input.vertex_id] * 2 - 1.0) * float2(vertex_input.rect_border_thickness, vertex_input.rect_border_thickness));; 
  float2 rect_vertex_in_px = rect_origin + (rect_vertex_coords[vertex_input.vertex_id] * rect_dims);

  // todo: Find a way to have this not take 3 lines 
  float2 rect_vertex_in_ndc = (rect_vertex_in_px / vp_dims) * 2.0;
  rect_vertex_in_ndc.x = rect_vertex_in_ndc.x - 1.0; 
  rect_vertex_in_ndc.y = 1.0 - rect_vertex_in_ndc.y;

  float rect_vertex_corner_r[4];
  rect_vertex_corner_r[0] = vertex_input.rect_corner_radius_00; rect_vertex_corner_r[1] = vertex_input.rect_corner_radius_10;
  rect_vertex_corner_r[2] = vertex_input.rect_corner_radius_01; rect_vertex_corner_r[3] = vertex_input.rect_corner_radius_11;
  for (uint i = 0; i < 4; i += 1) { rect_vertex_corner_r[i] = clamp(rect_vertex_corner_r[i], 0.0, 1.0); }

  PixelInput pixel_input;
  pixel_input.pos                  = float4(rect_vertex_in_ndc, 0, 1);
  pixel_input.rect_origin          = rect_origin;
  pixel_input.rect_dims            = rect_dims;
  pixel_input.corner_radius        = rect_vertex_corner_r[vertex_input.vertex_id];
  pixel_input.softness             = vertex_input.softness;
  pixel_input.border_thickness     = vertex_input.rect_border_thickness;
  pixel_input.vertex_color[UV__00] = vertex_input.rect_color_00;
  pixel_input.vertex_color[UV__10] = vertex_input.rect_color_10;
  pixel_input.vertex_color[UV__01] = vertex_input.rect_color_01;
  pixel_input.vertex_color[UV__11] = vertex_input.rect_color_11;

  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{
  float2 pos_px = pixel_input.pos.xy;

  float2 pos_norm = (pos_px - pixel_input.rect_origin) / pixel_input.rect_dims; 

  float4 top_color    = lerp(pixel_input.vertex_color[UV__00], pixel_input.vertex_color[UV__10], pos_norm.x);
  float4 bottom_color = lerp(pixel_input.vertex_color[UV__01], pixel_input.vertex_color[UV__11], pos_norm.x);
  float4 final_color  = lerp(top_color, bottom_color, pos_norm.y);

  // todo: Smooth the outside of the rect
  // -
  // todo: Smooth the inside of the rect after border
  //       The last 2 might be done all the time, but just doing the smoo the with the rect bound
  //       and then with the rect bound + border_width, i might be wrong

  {
    float radius_in_px = pixel_input.corner_radius * max(pixel_input.rect_dims.x, pixel_input.rect_dims.y) / 2.0;
    float sdf          = sdf_rounded_rect(pixel_input.rect_origin, pixel_input.rect_dims, pos_px, radius_in_px);

    if (sdf > 0.0) 
    {
      discard;
    }
    else if (-pixel_input.border_thickness <= sdf && sdf < 0.0)
    {
      final_color = float4(1, 1, 1, 1);
      float smoothed = smoothstep(0.0, pixel_input.softness, -sdf);
      final_color.a *= smoothed; 
    }

    if (-pixel_input.border_thickness <= sdf && sdf <= -(pixel_input.border_thickness - pixel_input.softness))
    {
      float smoothed = smoothstep(pixel_input.border_thickness, pixel_input.border_thickness-pixel_input.softness, -sdf);
      final_color.a *= smoothed; 
    }
  }

  return final_color;
}