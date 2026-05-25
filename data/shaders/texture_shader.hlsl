/* Some notes about text:
Text texture has to be made of white color and alpa. 
To color text tint is used. Since the letters are fully white,
the tint will make the whatever color the tint is.
For non font texture, the tint white is to be used to have the 
origin texture be drawn. 
*/

cbuffer constants : register(b0)
{
  float u_viewport_width_in_px;
  float u_viewport_height_in_px;
};

sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

struct VertexInput {
  float4 tint             : TINT;

  float2 dest_rect_origin : DEST_RECT_ORIGIN;
  float2 dest_rect_size   : DEST_RECT_SIZE;

  float2 src_rect_origin  : SRC_RECT_ORIGIN;
  float2 src_rect_size    : SRC_RECT_SIZE;

  float2 src_texture_dims : SRC_TEXTURE_DIMS;

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  float4 pos       : SV_POSITION;
  float2 source_uv : SOURCE_UV;
  float4 tint      : TINT;
};

PixelInput vs_main(VertexInput vertex_input) 
{
  float2 vp_dims = float2(u_viewport_width_in_px, u_viewport_height_in_px);
  
  float2 rect_vertex_coords[4] = {
    float2(0.0, 0.0), float2(1.0, 0.0),
    float2(0.0, 1.0), float2(1.0, 1.0),
  };

  // Getting dest rect ndc coords
  float2 desc_rect_vertex_in_px = vertex_input.dest_rect_origin + (rect_vertex_coords[vertex_input.vertex_id] * vertex_input.dest_rect_size);
  float2 desc_rect_vertex_in_ndc = (desc_rect_vertex_in_px / vp_dims) * 2.0;
  desc_rect_vertex_in_ndc.x = desc_rect_vertex_in_ndc.x - 1.0;
  desc_rect_vertex_in_ndc.y = 1.0 - desc_rect_vertex_in_ndc.y;

  // Getting source rect ndc coords
  float2 source_rect_vertex_in_px = vertex_input.src_rect_origin + (rect_vertex_coords[vertex_input.vertex_id] * vertex_input.src_rect_size);
  float2 source_rect_vertex_in_uv = source_rect_vertex_in_px / vertex_input.src_texture_dims;

  PixelInput pixel_input;
  pixel_input.pos       = float4(desc_rect_vertex_in_ndc, 0.0, 1.0);
  pixel_input.source_uv = source_rect_vertex_in_uv;
  pixel_input.tint      = vertex_input.tint;
  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{
  float4 texture_color = texture0.Sample(sampler0, pixel_input.source_uv);
  texture_color *= pixel_input.tint;
  return texture_color;
}

