// note: All these are to be bottom up
cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;

  float u_rect_origin_x_in_px; 
  float u_rect_origin_y_in_px; 

  float u_rect_width_in_px;
  float u_rect_height_in_px;

  int u_is_horizontal_gradient;
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

// note: Claude made this
float3 hsv_from_rgb(float3 c)
{
  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float4 ps_main(PS_Input input) : SV_TARGET
{
  float pos = (u_is_horizontal_gradient ? input.pos.x : input.pos.y);
  float hue = pos / (u_is_horizontal_gradient ? u_rect_width_in_px : u_rect_height_in_px);

  hue = frac(hue + 2.0/3.0);

  float3 rgb = hsv_from_rgb(float3(hue, 1.0, 1.0));
  return float4(rgb, 1.0);
}