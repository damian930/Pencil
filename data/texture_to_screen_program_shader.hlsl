sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

struct PS_INPUT {
  float4 pos: SV_POSITION;
  float2 uv : TEXCOORD;
};

PS_INPUT vs_main(uint id : SV_VertexID) 
{
  float2 pos;
  if      (id == 0) { pos = float2(-1, -1); }
  else if (id == 1) { pos = float2(-1, +1); }
  else if (id == 2) { pos = float2(+1, -1); }
  else if (id == 3) { pos = float2(+1, +1); }
  
  float2 uv = (pos + 1) * 0.5;
  
  PS_INPUT output;
  output.pos = float4(pos, 0, 1);
  output.uv = uv;
  return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
  float4 tex_color = texture0.Sample(sampler0, input.uv);      
  return tex_color;
}

