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

b64
MemMatches(u8 *A, u8 *B, u64 Length)
{
  b64 Result;

  while (*A == *B)
  {
    if (!--Length)
    {
      break;
    }

    ++A;
    ++B;
  }

  Result = (*A == *B);

  return (Result);
}

b64
MemMatches(char *A, char *B, u64 Length)
{
  return MemMatches((u8 *)A, (u8 *)B, Length);
}

u8 *
MemCopy(u8 *Source, u8 *Dest, u64 Length)
{
  u8 *Result = Dest;

  while (Length--)
  {
    *Dest++ = *Source++;
  }

  return (Result);
}

u8 *
MemSet(u8 *Dest, u8 Value, u64 Length)
{
  u8 *Result = Dest;

  while (Length--)
  {
    *Dest++ = Value;
  }

  return (Result);
}