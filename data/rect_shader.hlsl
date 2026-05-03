// note: All these are to be bottom up
cbuffer constants : register(b0)
{
  float u_vp_width_in_px;
  float u_vp_height_in_px;

  float u_rect_origin_x_in_px;
  float u_rect_origin_y_in_px;

  float u_rect_width_in_px;
  float u_rect_height_in_px;

  float u_border_thickness_in_px;

  float __pad1;

  float4 u_rect_color;
  float4 u_border_color;
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

Rect aply_border_to_rect(Rect rect, float border)
{
  Rect result_rect = rect;
  result_rect.x += border;
  result_rect.y += border;
  result_rect.width -= border;
  result_rect.height -= border;
  return result_rect;
}

bool is_point_inside_rect(Rect rect, float2 p)
{
  bool is_inside = (
		rect.x <= p.x && p.x < rect.x + rect.width  &&
		rect.y <= p.y && p.y < rect.y + rect.height
	);
	return is_inside;	
}

// todo: This v shader is 1 px off, since it does rect.x + rect.width for the left vertex,
//       but x + width is 1 out of the last rect position on the right.
//       I dont know how to get the last pos instead of the 1 afte the last pos.
// todo: Finish the pixel shader to have the border be a different color 

// todo: Make it so it works with the culling on 
PS_Input vs_main(uint id : SV_VertexID)
{
  Rect rect;
  rect.x      = u_rect_origin_x_in_px;
  rect.y      = u_rect_origin_y_in_px;
  rect.width  = u_rect_width_in_px;
  rect.height = u_rect_height_in_px;
  
  // Getting a vertex for the rect with the borders. Meaning that if the borders are outside the rect, 
  // then the rect is expanded and if they are negative and inside the rect, then the rect is the same
  float2 rect_vertex_in_px = float2(rect.x, rect.y);
  {
    float border_size = u_border_thickness_in_px;
  
    if (id == 0) 
    { 
      // Getting bottom left of the rect
      rect_vertex_in_px.y += rect.height;
      
      // Expanding the rect vertexes
      if (border_size > 0.0) 
      {  
        rect_vertex_in_px.x -= border_size;
        rect_vertex_in_px.y += border_size;
      } 
    }
    else if (id == 1) 
    { 
      // This is for the top left of the rect, rect.xy is already top left

      // Expanding the rect vertexes
      if (border_size > 0.0)
      {
        rect_vertex_in_px.x -= border_size;
        rect_vertex_in_px.y -= border_size;
      }
    }
    else if (id == 2) 
    { 
      // Getting bottom right of the rect
      rect_vertex_in_px.x += rect.width;
      rect_vertex_in_px.y += rect.height;
    
      // Expanding the rect vertexes
      if (border_size > 0.0)
      {
        rect_vertex_in_px.x += border_size;
        rect_vertex_in_px.y += border_size;
      }

      // Adjusting vertex pos based on border
    }
    else if (id == 3) { 
      // Getting top right of the rect
      rect_vertex_in_px.x += rect.width;

      // Expanding the rect vertexes
      if (border_size > 0.0)
      {
        rect_vertex_in_px.x += border_size;
        rect_vertex_in_px.y -= border_size;
      }
    }
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
  float4 result_color = u_border_color;

  Rect unbordered_rect = input.original_rect;
  if (u_border_thickness_in_px < 0.0)
  {
    unbordered_rect.x += -1.0 * u_border_thickness_in_px;
    unbordered_rect.y += -1.0 * u_border_thickness_in_px;
    unbordered_rect.width += 2 * u_border_thickness_in_px;
    unbordered_rect.height += 2 * u_border_thickness_in_px;
  }

  // Point is inside the unbordered part of the rect
  if (
    (unbordered_rect.x <= input.pos.x && input.pos.x < unbordered_rect.x + unbordered_rect.width) &&
    (unbordered_rect.y <= input.pos.y && input.pos.y < unbordered_rect.y + unbordered_rect.height)
  ) {
    result_color = u_rect_color;
  }

  return result_color;
}