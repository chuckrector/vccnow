
#include "vcc.hpp"
#include "compile.hpp"
#include "log.hpp"
#include "preproc.hpp"
#include "util.hpp"
#include "v1assets.hpp"
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
  strcpy_s(TempBuffer, TEMP_BUFFER_SIZE, FilenameWithoutExtension);
  u64 L = strlen(TempBuffer);
  if (L > 3 && !strcmp(".vc", FilenameWithoutExtension + (L - 3))) {}
  else
    strcpy_s(TempBuffer + strlen(TempBuffer), TEMP_BUFFER_SIZE, ".vc");
  CompileGuy.BasePath = (char *)NewTempBuffer(TEMP_BUFFER_SIZE);
  strcpy_s(CompileGuy.BasePath, TEMP_BUFFER_SIZE, TempBuffer);

  u8 *P = (u8 *)NewTempBuffer(WORKING_MEMORY_TOTAL_SIZE);
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
WriteScriptOffsetTableToBuffer(buffer *Output)
{
  // Log("WriteScriptOffsetTableToBuffer\n");
  u32 *P = (u32 *)Output->Data;
  *P++ = (u32)GlobalNumScripts;
  memcpy(P, GlobalScriptOffsetTable, 4 * GlobalNumScripts);
  Output->Length = 4 + (4 * GlobalNumScripts);
}

void
WriteMagicOutput()
{
  printf("WriteMagicOutput\n");
  FILE *f;

  fopen_s(&f, "magic.vcs", "wb");
  fwrite(&GlobalNumScripts, 1, 4, f);
  fwrite(GlobalScriptOffsetTable, 4, GlobalNumScripts, f);
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
  fwrite(&GlobalNumScripts, 1, 4, f);
  fwrite(GlobalScriptOffsetTable, 4, GlobalNumScripts, f);
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
  fwrite(&GlobalNumScripts, 1, 4, f);
  fwrite(&GlobalScriptOffsetTable, 4, GlobalNumScripts, f);
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
  // Log("WriteOutput %s\n", FilenameWithoutExtension);
  Assert(FilenameWithoutExtension);

  FILE *File;
  u16 MapWidth;
  u16 MapHeight;
  u32 NumEntities;
  u8 NumMovementScripts;
  u32 MovementScriptBufferSize;

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
    fopen_s(&File, MapFilename, "rb+");
    fseek(File, 68, 0);
    fread(&MapWidth, 1, 2, File);
    fread(&MapHeight, 1, 2, File);
    fseek(File, 100 + (MapWidth * MapHeight * 5) + 7956, 0);
    fread(&NumEntities, 1, 4, File);
    fseek(File, sizeof(v1entity) * NumEntities, 1);
    fread(&NumMovementScripts, 1, 1, File);
    fread(&MovementScriptBufferSize, 1, 4, File);
    fseek(File, (NumMovementScripts * 4) + MovementScriptBufferSize, 1);
  }
  else
  {
    char Temp[1024];
    sprintf_s(Temp, 1024, "%s.compiled", NoExt);
    printf("%s not found, falling back to %s\n", MapFilename, Temp);

    fopen_s(&File, Temp, "wb+");
  }

  u64 NumCompiledBytes =
      CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode;
  Log("Writing %lld scripts, %lld compiled bytes...",
      GlobalNumScripts,
      NumCompiledBytes);
  fwrite(&GlobalNumScripts, 1, 4, File);
  fwrite(GlobalScriptOffsetTable, 4, GlobalNumScripts, File);
  fwrite(CompileGuy.GeneratedCode, 1, NumCompiledBytes, File);
  Log("OK\n");
  fclose(File);
}
