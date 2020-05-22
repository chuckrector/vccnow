#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "mem.hpp"

struct buffer
{
  u8 *Data = 0;
  u64 Length = 0;
  u8 *C = 0;
  buffer() {}
  buffer(u8 *D, u64 L) : Data(D), Length(L) {}
  buffer(const char *S) : Data((u8 *)S), Length(strlen(S)) {}
  u16 GetW();
  u32 GetD();
};

buffer *NewBuffer(u8 *Data, u64 Length);
buffer *NewBuffer(const char *S);

struct mem_pool BufferPool;
void InitBufferPool();

#endif // BUFFER_HPP