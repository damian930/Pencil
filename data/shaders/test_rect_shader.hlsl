cbuffer cbuffer0 : register(b0) {
  float u_window_width;
  float u_window_height;
};

struct Rect {
  float x;
  float y;
  float width;
  float height;
};

struct CpuToVertex {
  float4 rect_color_00 : RECT_00_COLOR;  
  float4 rect_color_01 : RECT_01_COLOR;  
  float4 rect_color_10 : RECT_10_COLOR;  
  float4 rect_color_11 : RECT_11_COLOR;  

  float rect_origin_x : RECT_ORIGIN_X; 
  float rect_origin_y : RECT_ORIGIN_Y; 

  float rect_width    : RECT_WIDTH;
  float rect_height   : RECT_HEIGHT;
  
  uint id : SV_VertexID;
};

struct VertexToPixel {
  float4 pos      : SV_POSITION;
  float4 color_00 : FINAL_COLOR_00;
  float4 color_01 : FINAL_COLOR_01;
  float4 color_10 : FINAL_COLOR_10;
  float4 color_11 : FINAL_COLOR_11;
  Rect   rect     : RECT;
};


// todo: Make it so it works with the culling on 
VertexToPixel vs_main(CpuToVertex cpu_to_vertex) 
{
  Rect rect;
  rect.x      = cpu_to_vertex.rect_origin_x;
  rect.y      = cpu_to_vertex.rect_origin_y;
  rect.width  = cpu_to_vertex.rect_width;
  rect.height = cpu_to_vertex.rect_height;
  
  float2 rect_vertex_in_px = float2(rect.x, rect.y);
  if (cpu_to_vertex.id == 0) 
  { 
    // Getting bottom left of the rect
    rect_vertex_in_px.y += rect.height;
  }
  else if (cpu_to_vertex.id == 1) 
  { 
    // This is for the top left of the rect, rect.xy is already top left
  }
  else if (cpu_to_vertex.id == 2) 
  { 
    // Getting bottom right of the rect
    rect_vertex_in_px.x += rect.width;
    rect_vertex_in_px.y += rect.height;
  }
  else if (cpu_to_vertex.id == 3) { 
    // Getting top right of the rect
    rect_vertex_in_px.x += rect.width;
  }

  float2 rect_vertex_in_ndc = rect_vertex_in_px;
  rect_vertex_in_ndc.x = (rect_vertex_in_px.x / u_window_width) * 2.0 - 1.0; 
  rect_vertex_in_ndc.y = 1.0 - (rect_vertex_in_px.y / u_window_height) * 2.0;

  VertexToPixel v2p;
  v2p.pos      = float4(rect_vertex_in_ndc, 0, 1);
  v2p.color_00 = cpu_to_vertex.rect_color_00;
  v2p.color_01 = cpu_to_vertex.rect_color_01;
  v2p.color_10 = cpu_to_vertex.rect_color_10;
  v2p.color_11 = cpu_to_vertex.rect_color_11;
  v2p.rect     = rect;
  return v2p;
}

float4 ps_main(VertexToPixel vertex_to_pixel) : SV_TARGET
{
  float2 pos = vertex_to_pixel.pos.xy;

  float pos_uv_x = (pos.x - vertex_to_pixel.rect.x) / vertex_to_pixel.rect.width;
  float pos_uv_y = (pos.y - vertex_to_pixel.rect.y) / vertex_to_pixel.rect.height;

  float4 top_color    = lerp(vertex_to_pixel.color_00, vertex_to_pixel.color_10, pos_uv_x);
  float4 bottom_color = lerp(vertex_to_pixel.color_01, vertex_to_pixel.color_11, pos_uv_x);
  float4 final_color  = lerp(top_color, bottom_color, pos_uv_y);

  return final_color;
}