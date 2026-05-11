// note: All these are to be bottom up
cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;

  float u_rect_origin_x_in_px; 
  float u_rect_origin_y_in_px; 

  float u_rect_width_in_px;
  float u_rect_height_in_px;

  float4 u_top_color;
};

struct Rect {
  float x;
  float y;
  float width;
  float height;
};

struct PS_Input {
  float4 pos         : SV_POSITION;
  Rect original_rect : ORIGINAL_RECT;
};

// todo: Make it so it works with the culling on 
PS_Input vs_main(uint id : SV_VertexID)
{
  Rect rect;
  rect.x      = u_rect_origin_x_in_px;
  rect.y      = u_rect_origin_y_in_px;
  rect.width  = u_rect_width_in_px;
  rect.height = u_rect_height_in_px;
  
  // Getting a vertex for the rect with the borders
  float2 rect_vertex_in_px = float2(rect.x, rect.y);
  if (id == 0) 
  { 
    // Getting bottom left of the rect
    rect_vertex_in_px.y += rect.height;
  }
  else if (id == 1) 
  { 
    // This is for the top left of the rect, rect.xy is already top left
  }
  else if (id == 2) 
  { 
    // Getting bottom right of the rect
    rect_vertex_in_px.x += rect.width;
    rect_vertex_in_px.y += rect.height;
  }
  else if (id == 3) 
  { 
    // Getting top right of the rect
    rect_vertex_in_px.x += rect.width;
  }

  float2 rect_vertex_in_ndc = rect_vertex_in_px;
  rect_vertex_in_ndc.x = (rect_vertex_in_px.x / u_vp_width_in_px) * 2.0 - 1.0; 
  rect_vertex_in_ndc.y = 1.0 - (rect_vertex_in_px.y / u_vp_height_in_px) * 2.0;
  
  PS_Input result;
  result.pos           = float4(rect_vertex_in_ndc, 0, 1);
  result.original_rect = rect;

  return result;
}

float4 ps_main(PS_Input input) : SV_TARGET
{
  float2 pos = input.pos.xy;
  
  float ratio_x = (pos.x - u_rect_origin_x_in_px) / u_rect_width_in_px;
  float ratio_y = (pos.y - u_rect_origin_y_in_px) / u_rect_height_in_px;

  float4 white = float4(1.0, 1.0, 1.0, 1.0);
  float4 black = float4(0.0, 0.0, 0.0, 1.0);

  float4 top_color   = lerp(white, u_top_color, ratio_x);
  float4 final_color = lerp(top_color, black, ratio_y);

  return final_color;
}