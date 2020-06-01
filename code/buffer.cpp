#include "buffer.hpp"
#include "log.hpp"
#include "util.hpp"

void
InitBufferPool()
{
  Assert(!BufferPool.Base);
  InitMemPool(&BufferPool, PoolBase(BUFFER_POOL_INDEX), POOL_SIZE);
}

buffer *
NewBuffer(u8 *Data, u64 Length)
{
  Assert(BufferPool.Base);

  // Log("<<<NewBuffer %d, %lld %.*s>>>\n", BufferCount, Length, 20, Data);

  buffer *Result = NewItem(&BufferPool, buffer);
  Result->Data = Data;
  Result->C = Result->Data;
  Result->Length = Length;
  return Result;
}

buffer *
NewBuffer(char *S)
{
  return NewBuffer((u8 *)S, StringLength(S));
}

u16
buffer::GetW()
{
  u16 Result = *(u16 *)C;
  C += 2;
  return Result;
}

u32
buffer::GetD()
{
  u32 Result = *(u32 *)C;
  C += 4;
  return Result;
}
