cbuffer constants : register(b0)
{
  float4 u_color;

  // All of these are in screen px coordinates
  float u_circle_origin_x; 
  float u_circle_origin_y; 
  //
  float u_radius;
  //
  float u_window_width;
  float u_window_height;
}

struct PS_Input {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
  float2 circle_origin_in_px : ORIGIN;
  float circle_r_in_px : R;
};

PS_Input vs_main(uint id : SV_VertexID)
{
  // Here just create the 4 vertexes to draw with
  float2 v_pos_in_px;
  if      (id == 0) { v_pos_in_px = float2(u_circle_origin_x - u_radius, u_circle_origin_y + u_radius); } // bottom left
  else if (id == 1) { v_pos_in_px = float2(u_circle_origin_x - u_radius, u_circle_origin_y - u_radius); } // top left
  else if (id == 2) { v_pos_in_px = float2(u_circle_origin_x + u_radius, u_circle_origin_y + u_radius); } // bottom right
  else if (id == 3) { v_pos_in_px = float2(u_circle_origin_x + u_radius, u_circle_origin_y - u_radius); } // top right

  float2 v_pos_ndc = (v_pos_in_px / float2(u_window_width, u_window_height)) * 2.0 - 1.0;

  PS_Input result;
  result.pos   = float4(v_pos_ndc, 0, 1);
  result.color = u_color;
  result.circle_origin_in_px = float2(u_circle_origin_x, u_circle_origin_y);
  result.circle_r_in_px = u_radius;

  return result;
}

float4 ps_main(PS_Input input) : SV_TARGET
{
  input.pos.y = u_window_height - input.pos.y;
  float d = distance(input.pos.xy, input.circle_origin_in_px);
  float4 color = input.color;
  if (d > input.circle_r_in_px) { discard; }
  return color;
}