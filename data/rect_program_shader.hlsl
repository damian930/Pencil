// note: All these are to be bottom up
cbuffer constants : register(b0)
{
  float _rect_origin_x_in_px;
  float _rect_origin_y_in_px;

  float rect_width_in_px;
  float rect_height_in_px;

  float window_width_in_px;
  float window_height_in_px;

  uint red;
  uint green;
  uint blue;
  uint alpha;
}

struct PS_Input {
 float4 pos : SV_POSITION;
 float4 color : COLOR;
};

PS_Input vs_main(uint id : SV_VertexID)
{
  // origin here is the top left of the rect
  float rect_origin_x_in_px = _rect_origin_x_in_px;
  float rect_origin_y_in_px = _rect_origin_y_in_px;
  
  float2 rect_point_in_px = float2(rect_origin_x_in_px, rect_origin_y_in_px);
  if (id == 0) { // bottom left of the rect
    rect_point_in_px.y += rect_height_in_px;
  }
  else if (id == 1) { // top left of the rect
    // this is origin  
  }
  else if (id == 2) { // bottom right of the rect
    rect_point_in_px.x += rect_width_in_px;
    rect_point_in_px.y += rect_height_in_px;
  }
  else if (id == 3) { // top right of the rect
    rect_point_in_px.x += rect_width_in_px;
  }

  float2 final_pos = rect_point_in_px;
  final_pos.x = (rect_point_in_px.x / window_width_in_px) * 2.0 - 1.0; 
  final_pos.y = (rect_point_in_px.y / window_height_in_px) * 2.0 - 1.0f;
  
  PS_Input result;
  result.pos = float4(final_pos, 0, 1);
  result.color = float4(red, green, blue, alpha) / 255.0; 
  
  return result;
}

float4 ps_main(PS_Input input) : SV_TARGET
{
  return input.color; 
}