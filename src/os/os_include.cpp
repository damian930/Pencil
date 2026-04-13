#ifndef OS_INCLUDE_CPP
#define OS_INCLUDE_CPP

#include "core/core_include.h"

#if COMPILER_MSVC
  #include "os/win32/os_core_win32.cpp"
#else
  StaticAssert(0, "Cant include os .cpp file. This layer is not supported for this platform.");
#endif

#endif