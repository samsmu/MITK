#pragma once

#ifdef MITK_RECOVERABLE_ASSERTS
#include <cstdio>
#define MITK_RECOVERABLE_ASSERT(exp) do { if( !(exp) ) {\
  printf("Recoverable assert (%s) hit at %s:%d\n", #exp, __FILE__, __LINE__);\
  abort();\
} } while(0)
#else
#define MITK_RECOVERABLE_ASSERT(exp)
#endif

