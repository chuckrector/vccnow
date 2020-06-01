#ifndef MEM_HPP
#define MEM_HPP

#include "types.hpp"

#define POOL_SIZE (1024 * 1024 * 10)
#define MAX_POOLS 10
#define ALL_MEM_SIZE (POOL_SIZE * MAX_POOLS)
#define BUFFER_POOL_INDEX 0
#define TEMP_POOL_INDEX 1
#define MACRO_POOL_INDEX 2
#define TOKEN_POOL_INDEX 3
#define TOKEN_POOL_SIZE (POOL_SIZE * (MAX_POOLS - (TOKEN_POOL_INDEX + 1)))

void *AllMem;

struct mem_pool
{
  u8 *Base;
  u64 Size;
  u64 Used;
  u64 Residents;
};

void InitMemPool(mem_pool *Pool, u8 *Base, u64 Size);
void ResetPool(mem_pool *Pool);

#define NewItem(Pool, type) (type *)New_(Pool, sizeof(type))
#define NewList(Pool, Count, type) (type *)New_(Pool, (Count) * sizeof(type))
void *New_(mem_pool *Pool, u64 Size);
#define PoolBase(Index) ((u8 *)AllMem + (POOL_SIZE * (Index)))
u64 MemLeft(mem_pool *Pool);
b64 MemMatches(u8 *A, u8 *B, u64 Length);
b64 MemMatches(char *A, char *B, u64 Length);
u8 *MemCopy(u8 *Source, u8 *Dest, u64 Length);
u8 *MemSet(u8 *Dest, u8 Value, u64 Length);

#endif // MEM_HPP