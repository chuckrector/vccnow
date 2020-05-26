#ifndef UTIL_HPP
#define UTIL_HPP

#include "buffer.hpp"
#include "types.hpp"
#include <stdio.h>

#define Assert(Expression)                                                     \
  if (!(Expression))                                                           \
  {                                                                            \
    Log("%s line %d, assertion failed:\n%s\n",                                 \
        __FILE__,                                                              \
        __LINE__,                                                              \
        #Expression);                                                          \
    *(int *)0 = 0;                                                             \
  }

#define TEMP_BUFFER_SIZE (1024)
char *TempBuffer = 0;
mem_pool TempPool;
void InitTempPool();
char *NewTempBuffer(u64 Size);

b64 Exist(const char *Filename);
u64 FileSize(FILE *File);
buffer *LoadEntireFile(const char *Filename);
void SetPath(const char *Filename, char *Path, u64 PathLength);
void DumpHex(
    const char *Title,
    u8 *Buffer,
    u64 Length,
    u64 Limit = 8 * 8,
    char *Indent = "\t");
void FormatU64(u64 Num, char *Output);

#endif // UTIL_HPP