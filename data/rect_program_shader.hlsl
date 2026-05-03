// Uniforms
cbuffer constants : register(b0)
{
  // uniforms
}

// Things that propogates via the shader layers
struct PS_Input {
 float4 pos : SV_POSITION;

};

PS_Input vs_main(uint id : SV_VertexID)
{

}

float4 ps_main(PS_Input input) : SV_TARGET
{

}