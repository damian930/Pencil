cbuffer constants : register(b0)
{
  float rect_origin_x_in_px;
  float rect_origin_y_in_px;

  float rect_width_in_px;
  float rect_height_in_px;

  float window_width_in_px;
  float window_height_in_px;
}

float4 vs_main(uint id : SV_VertexID) : SV_Position
{
  const float2 rect_origin_in_px = float2(rect_origin_x_in_px, rect_origin_y_in_px);
  
  float2 rect_point_in_px = rect_origin_in_px;
  if (id == 0) { // bottom left of the rect
    rect_point_in_px.y += rect_height_in_px;   
  }
  else if (id == 1) { // top left of the rect
  
  }
  else if (id == 2) { // bottom right of the rect
    rect_point_in_px.x += rect_width_in_px;
    rect_point_in_px.y += rect_height_in_px;
  }
  else if (id == 3) { // top right of the rect
    rect_point_in_px.x += rect_width_in_px;
  }

  float2 final_pos; 
  final_pos.x = (rect_point_in_px.x / window_width_in_px) * 2.0 - 1.0; 
  final_pos.y = (rect_point_in_px.y / window_height_in_px) * 2.0 - 1.0; 
  final_pos.y *= -1;

  return float4(final_pos, 0, 1);
}

float4 ps_main() : SV_TARGET
{
  return float4(1, 1, 1, 1); 
}