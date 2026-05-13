cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;
};

sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

struct CpuToVertex {
  float4 text_color       : TEXT_COLOR;

  float2 dest_rect_origin : DEST_RECT_ORIGIN;
  float2 dest_rect_size   : DEST_RECT_SIZE;

  float2 src_rect_origin  : SRC_RECT_ORIGIN;
  float2 src_rect_size    : SRC_RECT_SIZE;

  float2 src_texture_dims : SRC_TEXTURE_DIMS;

  uint is_text_texture    : IS_TEXT_TEXTURE;

  uint id : SV_VertexID;
};

struct VertexToPixel {
  float4 pos           : SV_POSITION;
  float2 uv            : TEXCOORD;
  uint is_text_texture : IS_TEXT;
  float4 text_color    : TEXT_COLOR;
};

VertexToPixel vs_main(CpuToVertex cpu_to_vertex) 
{
  // Getting the vertex positions in the dest rtv in px space
  float2 v_pos_px;
  if      (cpu_to_vertex.id == 0) { v_pos_px = cpu_to_vertex.dest_rect_origin + float2(0.0,                            cpu_to_vertex.dest_rect_size.y); } // HDC bottom left
  else if (cpu_to_vertex.id == 1) { v_pos_px = cpu_to_vertex.dest_rect_origin + float2(0.0,                            0.0);                            } // HDC top left
  else if (cpu_to_vertex.id == 2) { v_pos_px = cpu_to_vertex.dest_rect_origin + float2(cpu_to_vertex.dest_rect_size.x, cpu_to_vertex.dest_rect_size.y); } // HDC bottom right
  else if (cpu_to_vertex.id == 3) { v_pos_px = cpu_to_vertex.dest_rect_origin + float2(cpu_to_vertex.dest_rect_size.x, 0.0);                            } // HDC top right

  // Getting uv in the src rtv in px space
  float2 uv_in_px;
  if      (cpu_to_vertex.id == 0) { uv_in_px = cpu_to_vertex.src_rect_origin + float2(0.0,                           cpu_to_vertex.src_rect_size.y); } 
  else if (cpu_to_vertex.id == 1) { uv_in_px = cpu_to_vertex.src_rect_origin + float2(0.0,                           0.0);                           } 
  else if (cpu_to_vertex.id == 2) { uv_in_px = cpu_to_vertex.src_rect_origin + float2(cpu_to_vertex.src_rect_size.x, cpu_to_vertex.src_rect_size.y); } 
  else if (cpu_to_vertex.id == 3) { uv_in_px = cpu_to_vertex.src_rect_origin + float2(cpu_to_vertex.src_rect_size.x, 0.0);                           } 

  // Pos fron px to ndc space
  float2 pos_ndc = (v_pos_px / float2(u_vp_width_in_px, u_vp_height_in_px)) * 2 - 1.0;
  pos_ndc.y *= -1;

  // Uv from px to ndc space
  float2 uv = uv_in_px / cpu_to_vertex.src_texture_dims;

  VertexToPixel output;
  output.pos             = float4(pos_ndc, 0, 1);
  output.uv              = uv;
  output.is_text_texture = cpu_to_vertex.is_text_texture;
  output.text_color      = cpu_to_vertex.text_color;
  return output;
}

float4 ps_main(VertexToPixel vertex_to_pixel) : SV_TARGET
{
  float4 tex_color = texture0.Sample(sampler0, vertex_to_pixel.uv);
  if (vertex_to_pixel.is_text_texture) { tex_color *= vertex_to_pixel.text_color; }      
  return tex_color;
}

