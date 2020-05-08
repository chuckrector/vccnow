#include "log.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
Fail(const char *Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
  // *(int *)0 = 0;
  exit(-1);
}

void
Log(const char *Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
}
