#ifndef __UI_H
#define __UI_H

#include "core/core_include.h"

#include "font_provider/font_provider.h"

/* todos:
  - If ther is a parent box and it has a single child and child fills the whole parent, 
    if we get actions for both of them, only parent will produce interacted with events.
    Not sure if its a problem, but think aobut it.
  - there are no assert right now it place that show that you use the ui bulding part of the ui core before ui begin the build process
*/

// typedef V2F32 (UI_text_measuring_ft) (Str8 text, Font font, F32 font_size);

enum UI_Size_kind {
  UI_Size_kind__px,
  UI_Size_kind__children_sum,
  UI_Size_kind__text,
  UI_Size_kind__percent_of_parent,
  // UI_Size_kind__em,
};

struct UI_Size {
  UI_Size_kind kind;
  F32 value;
  F32 strictness;
};

enum UI_Box_flag : U32 {
  UI_Box_flag__NONE               = (0 << 0),

  // note: testing this for now, but these might be a part of a single bigger thing
  UI_Box_flag__has_background    = (1 << 1),
  UI_Box_flag__has_corner_radius = (1 << 2),
  UI_Box_flag__has_borders       = (1 << 3),

  UI_Box_flag__has_text_contents = (1 << 4),

  UI_Box_flag__floating_x         = (1 << 5), 
  UI_Box_flag__floating_y         = (1 << 6), 

  // UI_Box_flag__clip_x = (1 << 7), 
  // UI_Box_flag__clip_y = (1 << 8), 

  UI_Box_flag__dont_draw_overflow_x = (1 << 9), 
  UI_Box_flag__dont_draw_overflow_y = (1 << 10), 

  UI_Box_flag__floating = UI_Box_flag__floating_x|UI_Box_flag__floating_y, 
  UI_Box_flag__dont_draw_overflow = UI_Box_flag__dont_draw_overflow_x|UI_Box_flag__dont_draw_overflow_y, 
};
typedef U32 UI_Box_flags;

// - Stacks for default settings
struct UI_Box_flags_node { UI_Box_flags v; UI_Box_flags_node* next; };
struct UI_Box_flgas_stack { UI_Box_flags_node* first; U64 count; B32 pop_after_first_use; };

struct UI_Layout_axis_node { Axis2 v; UI_Layout_axis_node* next; };
struct UI_Layout_axis_stack { UI_Layout_axis_node* first; U64 count; B32 pop_after_first_use; };

struct UI_Semantic_size_node { UI_Size v; UI_Semantic_size_node* next; };
struct UI_Semantic_size_stack { UI_Semantic_size_node* first; U64 count; B32 pop_after_first_use; };

// - Stacks for styles related to the shape of the box
struct UI_Color_node { V4F32 v; UI_Color_node* next; };
struct UI_Color_stack { UI_Color_node* first; U64 count; B32 pop_after_first_use; };

#define _MAKE_CORNER_(v) v.r.v[UV__00] = r00; v.r.v[UV__10] = r10; v.r.v[UV__01] = r01; v.r.v[UV__11] = r11; 

struct UI_Corner_radius_style {
  V4F32 r;
};
struct UI_Corner_radius_node { UI_Corner_radius_style v; UI_Corner_radius_node* next; };
struct UI_Corner_radius_stack { UI_Corner_radius_node* first; U64 count; B32 pop_after_first_use; };

// todo: Make this better looking 
UI_Corner_radius_style ui_corner_r_all(F32 f) 
{ 
  UI_Corner_radius_style v = {};
  v.r.v[UV__00] = f;
  v.r.v[UV__01] = f;
  v.r.v[UV__10] = f;
  v.r.v[UV__11] = f;
  return v;   
}

struct UI_Softness_node { F32 v; UI_Softness_node* next; };
struct UI_Softness_stack { UI_Softness_node* first; U64 count; B32 pop_after_first_use; };

// todo: If this is used, then move it out somewhere from here
struct UI_Border_style {
  F32 width;
  V4F32 color;
};
struct UI_Border_style_node { UI_Border_style v; UI_Border_style_node* next; };
struct UI_Border_style_stack { UI_Border_style_node* first; U64 count; B32 pop_after_first_use; };

// - Stacks for styles related to text
struct UI_Text_color_node { V4F32 v; UI_Text_color_node* next; };
struct UI_Text_color_stack { UI_Text_color_node* first; U64 count; B32 pop_after_first_use; }; 

struct UI_Text_font_node { FP_Font v; UI_Text_font_node* next; };
struct UI_Text_font_stack { UI_Text_font_node* first; U64 count; B32 pop_after_first_use; }; 

// struct UI_Text_font_size_node { F32 v; UI_Text_font_size_node* next; };
// struct UI_Text_font_size_stack { UI_Text_font_size_node* first; U64 count; B32 pop_after_first_use; }; 

// - Special stacks
// struct UI_ID_node { Str8 id; UI_ID_node* next; };
// struct UI_ID_stack { UI_ID_node* first; U64 count; B32 pop_after_first_use; };

// struct UI_Box_clip_data {
//   B32 is_found;
//   V2F32 on_screen_dims;
//   V2F32 content_dims;
//   V2F32* clip_offset;
// };

struct UI_Actions {
  // Basics
  B32 is_hovered;
  B32 is_down;
  B32 was_down;
  B32 left_box_while_was_down;

  // Composed
  B32 is_clicked;

  // Other
  F32 wheel_move;
};

struct UI_Box;
typedef void (*UI_Box_custom_draw_func_type) (UI_Box* box);

struct UI_Box {
  // Default box settings
  UI_Box_flags flags;
  Axis2 layout_axis;
  UI_Size semantic_size[Axis2__COUNT];

  struct {
    V4F32 color; 
    UI_Corner_radius_style corner_r;
    UI_Border_style border;
    F32 softness;
  } shape_style;

  struct {
    Str8 text;
    FP_Font font;
    F32 font_size;      
    V4F32 text_color;
  } text_style;

  UI_Box_custom_draw_func_type custom_draw_func;
  void* custom_draw_data;

  Str8 id; // Per build  
  B32 has_been_updated_this_build;

  // Final build data
  V2F32 final_on_screen_size; // Result of the sizing pass
  V2F32 final_parent_offset;  // Result of the relative to parent pass
  Rect final_on_screen_rect;  // These are the result of the finals layout pass 
  
  // Per build box tree
  UI_Box* first_child;
  UI_Box* last_child;
  UI_Box* next_sibling;
  UI_Box* prev_sibling;
  UI_Box* parent;
  U64 children_count;
};

struct UI_Box_data {
  B32 found;
  Rect on_screen_rect;
};

struct UI_Context {
  // Pesistent
  Arena* context_arena;
  // UI_text_measuring_ft* text_measuring_fp;
  Arena* build_arenas[2];
  U64 build_generation;

  // Per build
  UI_Box* root_box;
  UI_Box* current_parent_box;
  UI_Box* prev_frame_root_box; 

  // Per build
  F32 mouse_x;
  F32 mouse_y;

  // todo: Maybe dont call this "active", but rather "exclusive"
  Str8 currently_active_box_id; // This is for semantic ui meaning
  
  // === todo: I dont know how to explaine this fully even to myself at this moment --> this is for inputs, not semantic meaning 
  Str8 currently_interacted_with_box_id;
  B32 currently_interacted_with_box__is_down;
  B32 currently_interacted_with_box__left_box_while_was_down;

  Arena* style_stacks_arena; // Per build

  // Default style stacks
  UI_Box_flgas_stack     flags_stack;
  UI_Layout_axis_stack   layout_axis_stack;
  UI_Semantic_size_stack semantic_size_x_stack;
  UI_Semantic_size_stack semantic_size_y_stack;

  // Shape style stacks
  UI_Color_stack         color_stack;
  UI_Corner_radius_stack corner_radius_stack;
  UI_Border_style_stack  border_style_stack;
  UI_Softness_stack      softness_stack;

  // Text style stacks
  UI_Text_color_stack text_color_stack;
  UI_Text_font_stack text_font_stack;
  // UI_Text_font_size_stack text_font_size_stack;

  // Speciall stacks
  // UI_ID_stack id_stack;
};

// - Context variables
extern UI_Context* _ui_g_context;
extern UI_Box _ui_g_zero_box;
// V2F32 _ui_g_text_measuring_stub_f(Str8 text, Font font, U32 font_size);

// - Size makers
UI_Size ui_size_make(UI_Size_kind kind, F32 value, F32 strictness);
UI_Size ui_px(F32 value);                     
UI_Size ui_children_sum();                    
UI_Size ui_text_size();                       
UI_Size ui_p_of_p(F32 value, F32 strictness); 
// Just in case if i need these
// UI_Size ui_px_ex(F32 value, F32 strictness); 
// UI_Size ui_children_sum_ex(F32 strictness);  
// UI_Size ui_text_size_ex(F32 strictness);     

// - Context 
UI_Context* ui_get_context();
void ui_set_context(UI_Context* context);

// - Simple getters
Arena* ui_get_build_arena();
F32 ui_get_mouse_x();
F32 ui_get_mouse_y();
V2F32 ui_get_mouse_pos();

// - Context 
void ui_init();
void ui_release();

// - IDs
Str8 ui_get_text_part_from_str8(Str8 id_and_text);

// - Box stuff
B32 ui_box_is_zero(UI_Box* box);
UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags);
void ui_box_set_custom_draw(UI_Box* box, void (*draw_func) (UI_Box*), void* data);
void ui_push_parent(UI_Box* box);
void ui_pop_parent();
UI_Box* ui_get_parent();
#define UI_Parent(box) DeferLoop(ui_push_parent(box), ui_pop_parent())

// - Build
void ui_begin_build(F32 window_width, F32 window_height, F32 mouse_x, F32 mouse_y);
void ui_end_build();

// - UI agothirm
void ui_do_sizing_for_fixed_sized_box(UI_Box* root, Axis2 axis);
void ui_do_sizing_for_parent_dependant_box(UI_Box* root, Axis2 axis);
void ui_do_sizing_for_child_dependant_box(UI_Box* root, Axis2 axis);
void ui_do_layout_fixing(UI_Box* root, Axis2 axis);
void ui_do_relative_parent_offsets_for_box(UI_Box* root, Axis2 axis);
void ui_do_final_rect_for_box(UI_Box* root, Axis2 axis);
void ui_layout_box(UI_Box* root, Axis2 axis);

// - Active box stuff
B32 ui_is_active_id(Str8 box_id);
void ui_set_active_id(Str8 box_id);
void ui_reset_active_id_match(Str8 box_id);
//
B32 ui_is_active_box(UI_Box* box);
void ui_set_active_box(UI_Box* box);
void ui_reset_active_box_match(UI_Box* box);
//
B32 ui_has_active();
void ui_reset_active();

// - Other box data
UI_Box* ui_get_box_from_tree(UI_Box* root, Str8 id);
UI_Box* ui_get_box_prev_frame(Str8 id);
UI_Box_data ui_get_box_data_prev_frame_from_box(UI_Box* box);
UI_Box_data ui_get_box_data_prev_frame_from_id(Str8 id);

// UI_Box_clip_data ui_get_box_clip_data_prev_frame(Str8 id);

// - Actions
UI_Actions ui_actions_from_box(UI_Box* this_frames_box);
UI_Actions ui_actions_from_id(Str8 id);

// - Style stack operations for default settings
void ui_add_flags(UI_Box_flags flags);   void ui_add_flags_to_next(UI_Box_flags flags);
void ui_push_flags(UI_Box_flags v);      void ui_set_next_flags(UI_Box_flags v);      void ui_pop_flags();      void ui_auto_pop_flags_stack();      UI_Box_flags ui_peek_flags();      UI_Box_flags ui_get_flags();
void ui_push_layout_axis(Axis2 v);       void ui_set_next_layout_axis(Axis2 v);       void ui_pop_layout_axis(); void ui_auto_pop_layout_axis_stack(); Axis2   ui_peek_layout_axis(); Axis2   ui_get_layout_axis();
void ui_push_size_x(UI_Size v);          void ui_set_next_size_x(UI_Size v);          void ui_pop_size_x();      void ui_auto_pop_size_x_stack();      UI_Size ui_peek_size_x();      UI_Size ui_get_size_x();
void ui_push_size_y(UI_Size v);          void ui_set_next_size_y(UI_Size v);          void ui_pop_size_y();      void ui_auto_pop_size_y_stack();      UI_Size ui_peek_size_y();      UI_Size ui_get_size_y();            void ui_pop_size_y();       void ui_auto_pop_size_y_stack();       UI_Size      ui_peek_size_y();       UI_Size      ui_get_size_y();

#define UI_Flags(flags)        DeferLoop(ui_push_flags(flags),             ui_pop_flags()) 
#define UI_Layout_axis(axis2)  DeferLoop(ui_push_layout_axis(axis2),       ui_pop_layout_axis())
#define UI_SizeX(ui_size)      DeferLoop(ui_push_semantic_size_x(ui_size), ui_pop_semantic_size_x())
#define UI_SizeY(ui_size)      DeferLoop(ui_push_semantic_size_y(ui_size), ui_pop_semantic_size_y())

// - Style stack operations related to the shape of the box
void ui_push_color_no_flag(V4F32 v);
void ui_set_next_color_no_flag(V4F32 v);
void ui_pop_color();
void ui_auto_pop_color_stack();
V4F32 ui_peek_color();
V4F32 ui_get_color();
void ui_push_color(V4F32 v);
void ui_set_next_b_color(V4F32 v);

void ui_push_corner_r_no_flag(UI_Corner_radius_style v);
void ui_set_next_corner_r_no_flag(UI_Corner_radius_style v);
void ui_pop_corner_r();
void ui_auto_pop_corner_r_stack();
UI_Corner_radius_style ui_peek_corner_r();
UI_Corner_radius_style ui_get_corner_r();
void ui_push_corner_r(UI_Corner_radius_style v);
void ui_set_next_corner_r(UI_Corner_radius_style v);

void ui_push_border_no_flag(F32 width, V4F32 color);
void ui_set_next_border_no_flag(F32 width, V4F32 color);
void ui_pop_border();
void ui_auto_pop_border_stack();
UI_Border_style ui_peek_border();
UI_Border_style ui_get_border();
void ui_push_border(F32 width, V4F32 color);
void ui_set_next_border(F32 width, V4F32 color);

void ui_pop_softness();
void ui_auto_pop_softness();
F32 ui_peek_softness();
F32 ui_get_softness();
void ui_push_softness(F32 softness);
void ui_set_next_softness(F32 softness);

// note: These add flags as well to the next, might need a version for no flag style scopes
#define UI_Color(v)             DeferLoop(ui_push_b_color(v),           ui_pop_color())
#define UI_Border(width, color) DeferLoop(ui_push_border(width, color), ui_pop_border())
// #define UI_Corner_radius(v)    DeferLoop(ui_push_corner_radius(v), ui_pop_corner_radius())
// #define UI_Border_width(v)     DeferLoop(ui_push_border_width(v),  ui_pop_border_width())
// #define UI_Border_color(v)     DeferLoop(ui_push_border_color(v),  ui_pop_border_color())

// - Style stack operations for text
void ui_push_text_color(V4F32 v);
void ui_set_next_text_color(V4F32 v);
void ui_pop_text_color();
void ui_auto_pop_text_color_stack();
V4F32 ui_peek_text_color();
V4F32 ui_get_text_color();

void ui_push_font(FP_Font v);
void ui_set_next_font(FP_Font v);
void ui_pop_font();
void ui_auto_pop_font_stack();
FP_Font ui_peek_font();
FP_Font ui_get_font();

// void ui_push_font_size(F32 v);
// void ui_set_next_font_size(F32 v);
// void ui_pop_font_size();
// void ui_auto_pop_font_size_stack();
// F32 ui_peek_font_size();
// F32 ui_get_font_size();

// #define UI_TextColor(color) DeferLoop(ui_push_text_color(color), ui_pop_text_color())
// #define UI_Font(font)       DeferLoop(ui_push_font(font),        ui_pop_font())
// #define UI_FontSize(size)   DeferLoop(ui_push_font_size(size),   ui_pop_font_size())

// - Special stacks operations
// void ui_set_next_id(Str8 id); 
// Str8 ui_get_next_id();

// - private part of the api
static inline 
void __ui_clear_style_stacks()
{
  UI_Context* ctx = ui_get_context();

  ctx->flags_stack           = {};
  ctx->layout_axis_stack     = {};
  ctx->semantic_size_x_stack = {};
  ctx->semantic_size_y_stack = {};
  
  ctx->color_stack         = {};
  ctx->corner_radius_stack = {};
  ctx->border_style_stack  = {};
  ctx->softness_stack      = {};

  ctx->text_color_stack = {};
  ctx->text_font_stack  = {};
  // ctx->text_font_size_stack  = {};

  // ctx->id_stack = {};
}

static inline 
void __ui_push_defaults_onto_stacks()
{
  UI_Context* ctx = ui_get_context();

  ui_push_flags(0);
  ui_push_layout_axis(Axis2__y); 
  ui_push_size_x(ui_children_sum()); 
  ui_push_size_y(ui_children_sum());

  // todo: Change this to be transparent or settable by the outiside on build
  ui_push_color(v4f32(0.0f, 0.0f, 0.0f, 0.0f)); 
  ui_push_corner_r(ui_corner_r_all(0.0f));
  ui_push_border(0.0f, black());
  ui_push_softness(2.0f);

  ui_push_text_color(white());
  // ui_push_font(GetFontDefault()); // todo: Not the biggest fan of this line here
  // ui_push_font_size(32.0f);

  // { // Manually setting up the id stack, it is speciall, so manuall here is fine
  //   UI_ID_node* default_id = ArenaPush(ui_get_build_arena(), UI_ID_node);
  //   default_id->id = Str8{};
  //   StackPush(&ctx->id_stack, default_id);
  //   ctx->id_stack.count += 1;
  //   ctx->id_stack.pop_after_first_use = false;
  // }
}

/* List of things i think i have to be able to do with this ui for it to be ok --> 
  -      hover state for boxes
  -      box with a hover state
  -      box with a clicking logic
  -      box with hover and clicking logic 
  -      make a macro to be able to have stacks be generated, so you dont have
         to manage them in different places, just have a table like macro thing
  -      % of parent size
  - make a cool looking list of buttons
  - border style

  - some claude generated list of thing to be able to build with this system
      UI SYSTEM — COMPLEXITY LADDER
    ==============================

    TIER 1 — STATIC PRIMITIVES
    ---------------------------
    01. Text / Typography      Font scale, weight, color tokens. Headings, body, captions, code spans.
    02. Color Swatch           A box that is purely a color. The atom of your theme system.
    03. Divider                Horizontal/vertical rule. May carry a label.
    04. Spacer                 Invisible box that enforces spacing units.
    05. Icon                   SVG or glyph at a fixed size. Inherits color.
    06. Avatar                 Image or initials in a circle/square. Fixed sizes.
    07. Badge / Tag            Small pill with text and optional color variant.
    08. Spinner / Loader       Animated indicator of indeterminate progress.
    09. Skeleton               Placeholder shape while content loads.
    10. Image / Media Box      Constrained image with aspect ratio and object-fit.


    TIER 2 — INTERACTIVE ATOMS
    ---------------------------
    11. [x] - Button                 Primary, secondary, ghost, destructive. Disabled state. Icon slot.
    12. [ ] - Icon Button            Square button with only an icon. Needs tooltip.
    13. [ ] - Link                   Inline or standalone. Underline, hover, visited states.
    14. [x] - Checkbox               Checked, unchecked, indeterminate. Label slot.
    15. [x] - Radio                  Single selection from a group. Label slot.
    16. [+-] - Toggle / Switch        Binary on/off. Animated thumb.
    17. [ ] - Text Input             Single-line. Placeholder, label, helper, error states.
    18. [ ] - Textarea               Multi-line input. Auto-resize variant.
    19. [ ] - Select / Dropdown      Native or custom. Option list, placeholder, disabled.
    20. [ ] - Slider                 Range input. Single handle, optional value tooltip.


    TIER 3 — STATEFUL COMPONENTS
    -----------------------------
    21. Tooltip                Appears on hover/focus. Positioned relative to trigger.
    22. Popover                Floating panel anchored to a trigger. Dismissable.
    23. Accordion              Expand/collapse a section. Animated height.
    24. Tabs                   Switch between panels. Active indicator. Keyboard nav.
    25. Progress Bar           Determinate fill. Value, label, color variants.
    26. Alert / Banner         Info, success, warning, error. Dismissable.
    27. Toast / Snackbar       Timed notification. Stacking, dismiss, action.
    28. Modal / Dialog         Overlay with focus trap. Header, body, footer.
    29. Drawer / Sheet         Slides in from edge. Top, right, bottom, left.
    30. Chip / Tag Input       Add and remove tags inline within an input.
    31. File Upload            Drop zone + file list. Progress per file.
    32. Color Picker           Hue/saturation canvas + hex input.


    TIER 4 — COMPOSITE PATTERNS
    ----------------------------
    33. Card                   Surface with header, body, footer, media slot and actions.
    34. List / List Item       Virtualisable list. Icon, text, meta, action per row.
    35. Menu / Context Menu    Triggered list of actions. Groups, separators, icons.
    36. Command Palette        Search-driven action launcher. Keyboard-first.
    37. Combobox / Autocomplete  Input + filterable dropdown. Multi-select variant.
    38. Date Picker            Calendar grid + input. Range selection variant.
    39. Breadcrumb             Hierarchical path nav. Collapse on overflow.
    40. Pagination             Page controls with prev/next and jump-to.
    41. Table                  Sort, filter, row selection, sticky columns/header.
    42. Tree View              Nested hierarchy. Expand/collapse, selection.
    43. Stepper / Wizard       Multi-step flow. Linear or branching progress.
    44. Notification Center    List of past notifications. Read/unread state.


    TIER 5 — FULL SURFACES
    -----------------------
    45. Navigation Bar         Top or side. Logo, links, actions, mobile hamburger.
    46. Sidebar / Nav Rail     Collapsible. Active state, nested groups, icons + labels.
    47. Data Grid              Editable cells, column resize, row grouping, virtual scroll.
    48. Kanban Board           Drag-and-drop columns and cards. Add/edit inline.
    49. Rich Text Editor       Toolbar + editable area. Formatting, links, embeds.
    50. Form Builder           Dynamic form with validation, field groups, submit.
    51. Dashboard Layout       Grid of resizable, draggable widget tiles.
    52. Chat / Message Feed    Bubbles, timestamps, reactions, scroll-to-bottom.
    53. Calendar View          Month/week/day grid. Event placement, drag to reschedule.
    54. Settings Page          Sectioned form. Sidebar nav, save state, confirmation.
*/

// note: This is great to have the ui be independant.
//       Though i dont know if i like it. I have seen this used in clay.
//       I dont know how to just draw like ryan since i dont have 
//       a separated layer for drawing like i had before with raylib 
//       or how ryan has it with "D_" layer.
// ///////////////////////////////////////////////////////////
// // - Draw stuff for the ui for the ui
// //
// struct UI_Draw_command {
//   Rect rect;

//   V4F32 rect_color;
//   F32 border_width;
//   V4F32 border_color;

//   Str8 text;

//   UI_Draw_command* prev;
//   UI_Draw_command* next;
// };

// struct UI_Draw_command_list {
//   UI_Draw_command* first;
//   UI_Draw_command* last;
//   U64 count;
// };

// void ui_draw_command_from_ui_root(UI_Box* root);


#endif






