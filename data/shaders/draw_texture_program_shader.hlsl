sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;

  float2 u_origin_px;
  // float2 u_size_px;
};

struct PS_INPUT {
  float4 pos: SV_POSITION;
  float2 uv : TEXCOORD;
};

PS_INPUT vs_main(uint id : SV_VertexID) 
{
  float2 size_px = float2(5.0, 5.0);

  float2 v_pos_px;
  if      (id == 0) { v_pos_px = u_origin_px + float2(0.0,        size_px.y); } // HDC bottom left
  else if (id == 1) { v_pos_px = u_origin_px + float2(0.0,        0.0);       } // HDC top left
  else if (id == 2) { v_pos_px = u_origin_px + float2(size_px.x, size_px.y); } // HDC bottom right
  else if (id == 3) { v_pos_px = u_origin_px + float2(size_px.x, 0.0);       } // HDC top right

  float2 pos_ndc = (v_pos_px / float2(u_vp_width_in_px, u_vp_height_in_px)) * 2 - 1.0;
  pos_ndc.y *= -1;
  float2 uv      = (pos_ndc + 1) * 0.5;
  
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

