#ifndef __oe_base_util_atomics_H__
#define __oe_base_util_atomics_H__

// Use _WIN32 to detect both 32-bit and 64-bit Windows environments
#if defined(_WIN32) || defined(WIN32)
#if defined(__MINGW32__)
  // MinGW
#include "openeaagles/base/util/platform/atomics_mingw.hpp"
#else
  // Visual Studio (MSVC)
#include "openeaagles/base/util/platform/atomics_msvc.hpp"
#endif
#else
  // Linux GCC
#include "openeaagles/base/util/platform/atomics_linux.hpp"
#endif

#endif