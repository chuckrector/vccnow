#ifndef TYPES_HPP
#define TYPES_HPP

#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int64_t bool64;

struct Buffer_t
{
  u8 *Data = 0;
  u64 Length = 0;
  Buffer_t() {}
  Buffer_t(u8 *D, u64 L) : Data(D), Length(L) {}
  Buffer_t(const char *S) : Data((u8 *)S), Length(strlen(S)) {}
};

Buffer_t *NewBuffer(u8 *Data, u64 Length);
Buffer_t *NewBuffer(const char *S);
void InitBufferSlab();
bool64 IsBufferSlabReady = false;

#define ASSERT(Expression)                                                     \
  if (!(Expression))                                                           \
    Fail("%s@%d: Assertion failed: %s\n", __FILE__, __LINE__, #Expression);

#endif // TYPES_HPP