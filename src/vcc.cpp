
#include "vcc.hpp"
#include "compile.hpp"
#include "preproc.hpp"
#include "util.hpp"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
PostStartupFiles()
{
  if (CompileGuy.IsVerbose)
    printf("Reading source file into memory... \n");

  CompileGuy.Data = CompileGuy.DataPreproc;
}

void
PreStartupFiles(const char *FilenameWithoutExtension)
{
  // printf("!!! PreStartupFiles %s\n", FilenameWithoutExtension);
  strcpy_s(TempBuffer, TEMP_BUFFER_SIZE, FilenameWithoutExtension);
  u64 L = strlen(TempBuffer);
  if (L > 3 && !strcmp(".vc", FilenameWithoutExtension + (L - 3))) {}
  else
    strcpy_s(TempBuffer + strlen(TempBuffer), TEMP_BUFFER_SIZE, ".vc");
  CompileGuy.BasePath = new char[TEMP_BUFFER_SIZE];
  strcpy_s(CompileGuy.BasePath, TEMP_BUFFER_SIZE, TempBuffer);

  u8 *P = new u8[WORKING_MEMORY_TOTAL_SIZE];
  // memset(P, 0, WORKING_MEMORY_TOTAL_SIZE);
  CompileGuy.WorkingMemory = P;
  CompileGuy.Data = P + 0;
  CompileGuy.GeneratedCode = P + WORKING_MEMORY_BLOCK_SIZE;
  CompileGuy.DataPreproc = P + (WORKING_MEMORY_BLOCK_SIZE * 2);

  FILE *File;
  fopen_s(&File, TempBuffer, "rb");
  if (!File)
  {
    if (!CompileGuy.IsQuiet)
      printf("*error* Could not open input file: %s\n", TempBuffer);
    exit(-1);
  }

  if (CompileGuy.IsVerbose)
    printf("Reading source file into memory... \n");

  CompileGuy.Length = FileSize(File);
  fread(CompileGuy.Data, 1, CompileGuy.Length, File);
  CompileGuy.Data[CompileGuy.Length] = 0;
  fclose(File);
}

// NOTE(aen): Returns # of bytes written.
void
WriteScriptOffsetTableToBuffer(Buffer_t *Output)
{
  // Log("WriteScriptOffsetTableToBuffer\n");
  u32 *P = (u32 *)Output->Data;
  *P++ = (u32)numscripts;
  memcpy(P, scriptofstbl, 4 * numscripts);
  Output->Length = 4 + (4 * numscripts);
}

void
WriteMagicOutput()
{
  printf("WriteMagicOutput\n");
  FILE *f;

  fopen_s(&f, "magic.vcs", "wb");
  fwrite(&numscripts, 1, 4, f);
  fwrite(scriptofstbl, 4, numscripts, f);
  fwrite(
      CompileGuy.GeneratedCode,
      1,
      (CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode),
      f);
  fclose(f);

  if (Exist("error.txt"))
    remove("error.txt");
}

void
WriteEffectOutput()
{
  printf("WriteEffectOutput\n");
  FILE *f;

  fopen_s(&f, "effects.vcs", "wb");
  fwrite(&numscripts, 1, 4, f);
  fwrite(scriptofstbl, 4, numscripts, f);
  fwrite(
      CompileGuy.GeneratedCode,
      1,
      (CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode),
      f);
  fclose(f);

  if (Exist("error.txt"))
    remove("error.txt");
}

void
WriteScriptOutput()
{
  printf("WriteScriptOutput\n");
  FILE *f;

  fopen_s(&f, "startup.vcs", "wb");
  fwrite(&numscripts, 1, 4, f);
  fwrite(&scriptofstbl, 4, numscripts, f);
  fwrite(
      CompileGuy.GeneratedCode,
      1,
      (CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode),
      f);
  fclose(f);

  if (Exist("error.txt"))
    remove("error.txt");
}

void
WriteOutput(const char *FilenameWithoutExtension)
{
  // printf("WriteOutput %s\n", FilenameWithoutExtension);
  ASSERT(FilenameWithoutExtension);

  FILE *f;
  char i;
  short int mx, my;
  int a;

  char NoExt[1024];
  char VcFilename[1024];
  char MapFilename[1024];

  strcpy_s(NoExt, 1024, FilenameWithoutExtension);
  u64 L = strlen(NoExt);
  if (L > 3 && !strcmp(".vc", NoExt + (L - 3)))
  {
    NoExt[L - 3] = 0;
  }
  sprintf_s(VcFilename, "%s.vc", NoExt);
  sprintf_s(MapFilename, "%s.map", NoExt);

  if (Exist(MapFilename))
  {
    fopen_s(&f, MapFilename, "rb+");
    fseek(f, 68, 0);
    fread(&mx, 1, 2, f);
    fread(&my, 1, 2, f);
    fseek(f, 100 + (mx * my * 5) + 7956, 0);
    fread(&a, 1, 4, f);
    fseek(f, 88 * a, 1);
    fread(&i, 1, 1, f);
    fread(&a, 1, 4, f);
    fseek(f, (i * 4) + a, 1);
  }
  else
  {
    char Temp[1024];
    sprintf_s(Temp, 1024, "%s.compiled", NoExt);
    printf("%s not found, falling back to %s\n", MapFilename, Temp);

    fopen_s(&f, Temp, "wb+");
  }

  u64 NumCompiledBytes =
      CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode;
  Log("Writing %lld scripts, %lld compiled bytes...",
      numscripts,
      NumCompiledBytes);
  fwrite(&numscripts, 1, 4, f);
  fwrite(scriptofstbl, 4, numscripts, f);
  fwrite(CompileGuy.GeneratedCode, 1, NumCompiledBytes, f);
  Log("OK\n");
  fclose(f);

  // if (Exist("error.txt")) {
  //   printf("error.txt exists, removing...\n");
  // int Result = remove("error.txt");
  //   printf("result %d\n", Result);
  // }
}
