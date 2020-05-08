#include "types.hpp"
#include "log.hpp"

#define BUFFER_SLAB_SIZE 100
Buffer_t *BufferSlab = 0;
u64 BufferSlabResidents = 0;

void
InitBufferSlab()
{
  if (IsBufferSlabReady)
    return;
  BufferSlab = new Buffer_t[BUFFER_SLAB_SIZE];
  IsBufferSlabReady = true;
}
void
FreeBufferSlab()
{
  delete[] BufferSlab;
}

Buffer_t *
NewBuffer(u8 *Data, u64 Length)
{
  if (!IsBufferSlabReady)
    Fail("Error: Must call InitBufferSlab before using NewBuffer\n");
  if (BufferSlabResidents >= BUFFER_SLAB_SIZE)
    Fail("Too many slab buffers! Over %d, exiting...\n", BUFFER_SLAB_SIZE);
  // Log("<<<NewBuffer %d>>>\n", BufferSlabResidents);
  Buffer_t *Result = BufferSlab + BufferSlabResidents++;
  Result->Data = Data;
  Result->Length = Length;
  return Result;
}

Buffer_t *
NewBuffer(const char *S)
{
  return NewBuffer((u8 *)S, strlen(S));
}