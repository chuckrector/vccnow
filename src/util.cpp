#include "util.hpp"
#include "mem.hpp"
#include <stdio.h>

void
InitTempPool()
{
  Assert(!TempPool.Base);
  InitMemPool(&TempPool, PoolBase(TEMP_POOL_INDEX), POOL_SIZE);
}

char *
NewTempBuffer(u64 L)
{
  Assert(TempPool.Base);
  char *Result = NewList(&TempPool, L, char);
  return Result;
}

b64
Exist(const char *Filename)
{
  FILE *f;

  fopen_s(&f, Filename, "rb");
  if (f)
  {
    fclose(f);
    return 1;
  }

  return 0;
}

u64
FileSize(FILE *File)
{
  u64 CurrentPosition = ftell(File);
  fseek(File, 0L, SEEK_END);
  u64 Size = ftell(File);
  fseek(File, (long)CurrentPosition, SEEK_SET);
  return Size;
}

buffer *
Load(const char *Filename)
{
  // Log("Load %s...\n", Filename);
  if (!Exist(Filename))
    Fail("Error: File does not exist: %s\n", Filename);
  FILE *File;
  fopen_s(&File, Filename, "rb");
  if (!File)
    Fail("Error: Unable to open file %s\n", Filename);
  u64 L = FileSize(File);
  u8 *D = (u8 *)NewTempBuffer(L + 1);
  fread(D, 1, L, File);
  fclose(File);

  // NOTE(aen): Not strictly necessary, but the original V1 VC compiler expected
  // file text to be null-terminated. If it's not, it will warn about unknown
  // tokens outside scripts.
  D[L] = 0;

  // Log("Loaded %d bytes\n", L);
  return NewBuffer(D, L);
}

void
SetPath(const char *Filename, char *Path, u64 PathLength)
{
  char FullPath[_MAX_PATH];
  _fullpath(FullPath, Filename, _MAX_PATH);

  char Drive[_MAX_DRIVE];
  char Directory[_MAX_PATH];
  _splitpath_s(
      FullPath, Drive, _MAX_DRIVE, Directory, _MAX_PATH, NULL, 0, NULL, 0);

  sprintf_s(Path, PathLength, "%s%s", Drive, Directory);
}

void
DumpHex(const char *Title, u8 *Buffer, u64 Length, u64 Limit, char *Indent)
{
  Log("%s\n%s", Title, Indent);
  if (!Length)
  {
    Log("<empty>\n");
    return;
  }
  int N = 0;
  for (; N < Length && N < Limit; N++)
  {
    if (N && !(N % 8))
      Log("\n%s", Indent);
    Log("%02x ", Buffer[N]);
  }
  if (N < Length)
    Log("\n%s(%lld more)\n", Indent, Length - N);
  Log("\n");
}

void
FormatU64(u64 Num, char *Output)
{
  char Temp[1024];
  sprintf_s(Temp, 1024, "%lld", Num);
  u64 L = strlen(Temp);
  u64 NumCommas = (L - 1) / 3;
  Output[L + NumCommas] = 0;
  for (u64 i = 0; i < L + NumCommas; i++)
    Output[i] = ' ';
  char *T1 = Temp + L - 1;
  char *T2 = Output + L + NumCommas - 1;
  while (L > 3)
  {
    for (u64 i = 0; i < 3; i++)
    {
      *T2-- = *T1--;
    }
    L -= 3;
    *T2-- = ',';
  }
  while (L--)
    *T2-- = *T1--;
}
