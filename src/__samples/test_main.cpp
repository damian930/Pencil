#define _CRT_SECURE_NO_WARNINGS

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "__third_party/stb/stb_image_write.h"
#endif

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

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

int main()
{
  allocate_thread_context();
  os_init();
  // r_init(); 
  // d_init();
  // fp_init();
  // ui_init();

  // void* r_mem = os_mem_reserve(Kilobytes(64), true, 10);
  // void* c_mem = os_mem_commit(r_mem, Kilobytes(4));
  // U32* test = (U32*)r_mem;
  // *test = 5;
  BP;

  return 0;
}