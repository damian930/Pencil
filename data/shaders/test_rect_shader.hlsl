cbuffer cbuffer0 : register(b0) {
  float u_window_width;
  float u_window_height;
};

struct VertexInput {
  float4 rect_color_00 : RECT_00_COLOR;  
  float4 rect_color_10 : RECT_10_COLOR;  
  float4 rect_color_01 : RECT_01_COLOR;  
  float4 rect_color_11 : RECT_11_COLOR;  

  float rect_origin_x : RECT_ORIGIN_X; 
  float rect_origin_y : RECT_ORIGIN_Y; 

  float rect_width    : RECT_WIDTH;
  float rect_height   : RECT_HEIGHT;

  float rect_corner_radius_00 : RECT_00_CORNER_RADIUS;
  float rect_corner_radius_10 : RECT_10_CORNER_RADIUS;
  float rect_corner_radius_01 : RECT_01_CORNER_RADIUS;
  float rect_corner_radius_11 : RECT_11_CORNER_RADIUS;

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  float4 color_00 : FINAL_COLOR_00;
  float4 color_10 : FINAL_COLOR_10;
  float4 color_01 : FINAL_COLOR_01;
  float4 color_11 : FINAL_COLOR_11;
  float2 rect_origin : RECT_ORIGIN;
  float2 rect_dims : RECT_DIMS;
  float corner_radius : CORNER_R;
  float4 pos      : SV_POSITION;
};

float sdf_rounded_rect(float2 rect_origin, float2 rect_dims, float2 p, float r)
{
  float2 rect_max_point = rect_origin + rect_dims;
  float2 rect_half_dims = rect_dims / 2.0;
  float2 rect_mid       = (rect_max_point + rect_origin) / 2.0;
  
  float2 diff = abs(p - rect_mid) - rect_half_dims + float2(r, r); 
	return length(max(diff, 0.0)) + min(max(diff.x, diff.y), 0.0) - r;
}

// todo: Make it so you can round the corners and make and oval or a circle with this
// todo: Add borders back in here, round them as well

// todo: Make it so it works with the culling on 
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

  // todo: Find a way to have this not take 3 lines 
  float2 rect_vertex_in_ndc = (rect_vertex_in_px / vp_dims) * 2.0;
  rect_vertex_in_ndc.x = rect_vertex_in_ndc.x - 1.0; 
  rect_vertex_in_ndc.y = 1.0 - rect_vertex_in_ndc.y;

  float rect_vertex_corner_r[4];
  rect_vertex_corner_r[0] = vertex_input.rect_corner_radius_00; rect_vertex_corner_r[1] = vertex_input.rect_corner_radius_10;
  rect_vertex_corner_r[2] = vertex_input.rect_corner_radius_01; rect_vertex_corner_r[3] = vertex_input.rect_corner_radius_11;

  for (uint i = 0; i < 4; i += 1) { rect_vertex_corner_r[i] = clamp(rect_vertex_corner_r[i], 0.0, 1.0); }

  PixelInput pixel_input;
  pixel_input.pos           = float4(rect_vertex_in_ndc, 0, 1);
  pixel_input.color_00      = vertex_input.rect_color_00;
  pixel_input.color_10      = vertex_input.rect_color_10;
  pixel_input.color_01      = vertex_input.rect_color_01;
  pixel_input.color_11      = vertex_input.rect_color_11;
  pixel_input.rect_origin   = rect_origin;
  pixel_input.rect_dims     = rect_dims;
  pixel_input.corner_radius = rect_vertex_corner_r[vertex_input.vertex_id];
  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{
  float2 pos_px = pixel_input.pos.xy;

  float2 pos_norm = (pos_px - pixel_input.rect_origin) / pixel_input.rect_dims; 

  float4 top_color    = lerp(pixel_input.color_00, pixel_input.color_10, pos_norm.x);
  float4 bottom_color = lerp(pixel_input.color_01, pixel_input.color_11, pos_norm.x);
  float4 final_color  = lerp(top_color, bottom_color, pos_norm.y);

  float radius_in_px = pixel_input.corner_radius * max(pixel_input.rect_dims.x, pixel_input.rect_dims.y) / 2.0;
  float sdf = sdf_rounded_rect(pixel_input.rect_origin, pixel_input.rect_dims, pos_px, radius_in_px);
  float sdf_factor = 1.f - smoothstep(0, 2*1.0f, sdf);
  final_color *= sdf_factor;
  return final_color;
}