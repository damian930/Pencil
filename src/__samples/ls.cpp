#define _CRT_SECURE_NO_WARNINGS

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "__third_party/stb/stb_image_write.h"
#endif

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

int main(int argc, char** argv)
{
  allocate_thread_context();
  os_init();

  Arena* arena = arena_alloc(Kilobytes(4), false, 0);

  if (argc > 2) { printf("ls: too many arguments passed in. \n"); }

  Str8 path_to_ls = {};
  if (argc == 1) { path_to_ls = os_get_current_dir_path(arena); }
  else if (argc == 2) 
  {
    Str8 path = str8_from_cstr(arena, (U8*)argv[1]);
    if (str8_match(path, Str8FromC("."), 0)) { path_to_ls = os_get_current_dir_path(arena); } 
    else { path_to_ls = path; }
  } 

  WIN32_FIND_DATAA data = {};
  HANDLE handle = FindFirstFileA((char*)path_to_ls.data, &data);
  printf("%s \n", data.cFileName);

  for (;;)
  {
    B32 succ = FindNextFileA(handle, &data);
    if (!succ) { break; }
    printf("%s \n", data.cFileName);
  }


  return 0;

  // todo: if nothing is passed then we use the current pat
  // todo: if "." then we use the current pass
  // todo: if there is a path then we have to use something else
  // todo: There can only be a single arg passed in there

}
