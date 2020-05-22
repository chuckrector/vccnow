#include "mem.hpp"
#include "log.hpp"
#include "types.hpp"

void
InitMemPool(mem_pool *Pool, u8 *Base, u64 Size)
{
  Pool->Base = Base;
  Pool->Size = Size;
  Pool->Used = 0;
  Pool->Residents = 0;
}

void *
New_(mem_pool *Pool, u64 Size)
{
  if (Pool->Used + Size > Pool->Size)
  {
    Log("Requested %lld bytes. Overshot pool size by %lld bytes.",
        Size,
        (Pool->Used + Size) - Pool->Size);
  }
  Assert((Pool->Used + Size) <= Pool->Size);
  void *Result = Pool->Base + Pool->Used;
  Pool->Used += Size;
  Pool->Residents++;
  return Result;
}

// NOTE(aen): Mainly for testing; allocate maxes to verify they work, then reset
// so that tokenization, macro expansion, decompilation, etc. works.
void
ResetPool(mem_pool *Pool)
{
  Pool->Used = 0;
  Pool->Residents = 0;
}

u64
MemLeft(mem_pool *Pool)
{
  return Pool->Size - Pool->Used;
}