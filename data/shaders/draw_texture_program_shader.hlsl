sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;

  float2 u_dest_rect_origin;
  float2 u_dest_rect_size;

  float2 u_src_rect_origin;
  float2 u_src_rect_size;

  float2 u_src_texture_dims;
};

struct PS_INPUT {
  float4 pos: SV_POSITION;
  float2 uv : TEXCOORD;
};

PS_INPUT vs_main(uint id : SV_VertexID) 
{
  // Getting the vertex positions in the dest rtv in px space
  float2 v_pos_px;
  if      (id == 0) { v_pos_px = u_dest_rect_origin + float2(0.0,        u_dest_rect_size.y);         } // HDC bottom left
  else if (id == 1) { v_pos_px = u_dest_rect_origin + float2(0.0,        0.0);                        } // HDC top left
  else if (id == 2) { v_pos_px = u_dest_rect_origin + float2(u_dest_rect_size.x, u_dest_rect_size.y); } // HDC bottom right
  else if (id == 3) { v_pos_px = u_dest_rect_origin + float2(u_dest_rect_size.x, 0.0);                } // HDC top right

  // Getting uv in the src rtv in px space
  float2 uv_in_px;
  if      (id == 0) { uv_in_px = u_src_rect_origin + float2(0.0,        u_src_rect_size.y);         } 
  else if (id == 1) { uv_in_px = u_src_rect_origin + float2(0.0,        0.0);                       } 
  else if (id == 2) { uv_in_px = u_src_rect_origin + float2(u_src_rect_size.x, u_src_rect_size.y);  } 
  else if (id == 3) { uv_in_px = u_src_rect_origin + float2(u_src_rect_size.x, 0.0);                } 

  // Pos fron px to ndc space
  float2 pos_ndc = (v_pos_px / float2(u_vp_width_in_px, u_vp_height_in_px)) * 2 - 1.0;
  pos_ndc.y *= -1;

  // Uv from px to ndc space
  float2 uv = uv_in_px / u_src_texture_dims;

  PS_INPUT output;
  output.pos = float4(pos_ndc, 0, 1);
  output.uv  = uv;
  return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
  float4 tex_color = texture0.Sample(sampler0, input.uv);      
  return tex_color;
}

