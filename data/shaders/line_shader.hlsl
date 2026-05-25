cbuffer cbuffer0 : register(b0) {
  float u_window_width;
  float u_window_height;
};

struct VertexInput {
  float4 line_color : LINE_COLOR;  

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  nointerpolation float4 line_color : LINE_COLOR;
  float4 pos : SV_POSITION;
};

#define UV__x0y0  0
#define UV__x1y0  1
#define UV__x0y1  2
#define UV__x1y1  3
#define UV__COUNT 4 

#define PI 3.1415926535

float radians_from_degrees(float degrees)
{
	return degrees * (PI / 180.0);
}

/* NOTE:
  - It was taking too much time, i didnt want to use ai for it, os i am just not gon use it now.
    I am bad at math, it will take a lot of time, its not worth it.
*/

PixelInput vs_main(VertexInput vertex_input) 
{
  float2 rect_vertex_coords[UV__COUNT] = {
    float2(0.0, 0.0), float2(1.0, 0.0),
    float2(0.0, 1.0), float2(1.0, 1.0),
  };
  float2 viewport_dims = float2(u_window_width, u_window_height);

  float2 rect_origin = float2(50, 50);
  float2 rect_dims   = float2(100, 200);

  float2 rect_vertex_in_px = rect_origin + (rect_dims * rect_vertex_coords[vertex_input.vertex_id]);
  float2 rect_vertex_in_ndc = (rect_vertex_in_px / viewport_dims) * 2.0;
  rect_vertex_in_ndc.x = rect_vertex_in_ndc.x - 1.0; 
  rect_vertex_in_ndc.y = 1.0 - rect_vertex_in_ndc.y;

  // Rotating by 90 degrees
  float degrees = 270.0;
  float2x2 rotation_matrix_around_z_axis = float2x2(cos(radians_from_degrees(degrees)), -sin(radians_from_degrees(degrees)),
                                                    sin(radians_from_degrees(degrees)), cos(radians_from_degrees(degrees)));
  rect_vertex_in_ndc = mul(rect_vertex_in_ndc, rotation_matrix_around_z_axis);

  PixelInput pixel_input;
  pixel_input.pos        = float4(rect_vertex_in_ndc, 0, 1); 
  pixel_input.line_color = vertex_input.line_color;
  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{


  return pixel_input.line_color;
}