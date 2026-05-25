#define _CRT_SECURE_NO_WARNINGS

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "__third_party/stb/stb_image_write.h"
#endif

#include "core/core_include.h"
#include "core/core_include.cpp"

// #include "os/win32.h"
// #include "os/win32.cpp"

// #include "render/render.h"
// #include "render/render.cpp"

// #include "draw/draw.h"
// #include "draw/draw.cpp"

// #include "font_provider/font_provider.h"
// #include "font_provider/font_provider.cpp"

// #include "ui/ui_core.h"
// #include "ui/ui_core.cpp"

// #include "render/render.h"
// #include "render/render.cpp"

// #include "ui/widgets/ui_widgets.h"
// #include "ui/widgets/ui_widgets.cpp"

void test(U32* arr)
{
  
}

struct Ball {
  V4F32 color;
};

struct State {
  U64* p;
  Ball balls[1000];
  U64 count;
};

void add_ball(State* S)
{
  if (S->count < ArrayCount(S->balls))
  {
    Ball* ball = S->balls + S->count;
    ball->color = blue();
    S->count += 1;
  }
}

int main()
{
  allocate_thread_context();

  State state = {};
  add_ball(&state);
  add_ball(&state);
  add_ball(&state);
  add_ball(&state);

  return 0;
}