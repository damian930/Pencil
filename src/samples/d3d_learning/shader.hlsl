struct VS_INPUT
{
//   // float4 position : POSITION; // Matches the input layout semantic
  // float4 color : COLOR;
};

struct PS_INPUT
{
  float4 position : SV_POSITION; // SV_POSITION is required by the rasterizer
  float4 color : COLOR;
};

PS_INPUT vs_main(uint vertex_id: SV_VertexID)
{
  float4 pos;
  float4 color;
  if      (vertex_id == 0) { pos = float4(-1.0f, +1.0f, 0.5f, 1.0f); color = float4(1, 1, 1, 1);  }
  else if (vertex_id == 1) { pos = float4(-1.0f, -1.0f, 0.5f, 1.0f); color = float4(0, 0, 0, 1);  }
  else {
    pos = float4(+1.0f, -1.0f, 0.5f, 1.0f);
    color = float4(1, 0, 1, 1); 
  }

  PS_INPUT output;
  output.position = pos;
  output.color = color;
  return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET 
{
  return input.color;
}