#include "log.hpp"
#include "types.hpp"
#include <stdarg.h>

void
Fail(char *Format, ...)
{
  printf("FAIL\n");
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
  *(int *)0 = 0;
  // exit(-1);
}

void
Log(char *Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
}

void
DebugLog(DebugLevel_e Level, char *Format, ...)
{
  if (Level > DebugLevel)
    return;
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
}
