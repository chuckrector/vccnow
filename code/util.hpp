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

b64 Exist(char *Filename);
u64 FileSize(FILE *File);
buffer *LoadEntireFile(char *Filename);
void SetPath(char *Filename, char *Path, u64 PathLength);
void DumpHex(
    char *Title,
    u8 *Buffer,
    u64 Length,
    u64 Limit = 8 * 8,
    char *Indent = "\t");
void FormatU64(u64 Num, char *Output);
u32
SafeTruncateU64(u64 Value)
{
  Assert(Value <= 0xffffffff);
  u32 Result = (u32)Value;
  return (Result);
}

u32 SafeTruncateU64(u64 Value);

#endif // UTIL_HPP