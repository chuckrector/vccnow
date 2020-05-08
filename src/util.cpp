#include "util.hpp"
#include <stdio.h>

// void strupr(char *s) {
//   while (*s) {
//     if (*s >= 'a' && *s <= 'z') *s -= 32;
//     s++;
//   }
// }

bool64
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

Buffer_t *
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
  u8 *D = new u8[L + 1];
  fread(D, 1, L, File);
  fclose(File);
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