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
  OS_Mem_chunk mem_chunk = os_reserve_mem_chunck(2, false, 0);
  B32 succ = os_commit_mem_pages_to_chunck(&mem_chunk, 1);
  // B32 succ2 = os_commit_mem_pages_to_chunck(&mem_chunk, 1);
  for EachIndex(i, mem_chunk.n_pages_commited * os_get_mem_page_size() + 1)
  {
    if (i == os_get_mem_page_size()) { os_commit_mem_pages_to_chunck(&mem_chunk, 1); }
    U8* byte = (U8*)mem_chunk.base_p + i;
    *byte = (U8)i;
  }
  BP;
  return 0;
}