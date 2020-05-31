#include "decompile.cpp"

#include "buffer.cpp"
#include "compile.cpp"
#include "funclib.cpp"
#include "lib_funcs.cpp"
#include "log.cpp"
#include "log.hpp"
#include "mem.cpp"
#include "nichgvc.cpp"
#include "preproc.cpp"
#include "ricvc.cpp"
#include "runtests.cpp"
#include "util.cpp"
#include "v1assets.hpp"
#include "v1vc_macro.cpp"
#include "v1vc_parser.cpp"
#include "v1vc_token.cpp"
#include "vcc.cpp"

#include <malloc.h>
#include <stdlib.h>

#define BUILD 1

int
main(int ArgCount, char *ArgValues[])
{
  if (ArgCount >= 2)
  {
    switch (ArgValues[1][1])
    {
      case 'l':
        Log("DebugLevel LOW\n");
        DebugLevel = LOW;
        break;
      case 'm':
        Log("DebugLevel MEDIUM\n");
        DebugLevel = MEDIUM;
        break;
      case 'h':
        Log("DebugLevel HIGH\n");
        DebugLevel = HIGH;
        break;
      case 'v': CompileGuy.IsVerbose = true; break;
      case 'q': CompileGuy.IsQuiet = true; break;
    }
  }

  b64 DoTests = false;
  if (ArgCount == 2 && ArgValues[1][0] == 't')
  {
    DoTests = true;
  }
  else if (ArgCount != 3)
  {
    Log("vccnow, Build %d\n\n", BUILD);
    Log("Usage:\n");
    Log("  vccnow <mode> <file>\n\n");
    Log("Modes:\n");
    Log("  a - disassemble\n");
    Log("  c - compile (writes to file.compiled if no file.map found)\n");
    Log("  d - decompile (can be file.map or file.compiled)\n");
    Log("  p - preprocess\n");
    Log("  t - tests (no file argument)\n");
    Log("  v - validate script offset table\n");
    Log("  x - extract compiled script from map\n\n");
    Log("Mode modifiers:\n");
    Log("  q - quiet (legacy)\n");
    Log("  v - verbose (legacy)\n");
    Log("  l - low debug logging\n");
    Log("  m - medium debug logging\n");
    Log("  h - high debug logging\n\n");
    Log("Examples:\n");
    Log("  vccnow c test.vc\n");
    Log("  vccnow al test.compiled\n");
    Log("  vccnow th\n");
    exit(-1);
  }

  // NOTE(aen): This is the only memory used by the entire app.
  AllMem = malloc(ALL_MEM_SIZE);
  memset(AllMem, 0, ALL_MEM_SIZE);

  InitTempPool();
  TempBuffer = NewTempBuffer(TEMP_BUFFER_SIZE);
  InitBufferPool();
  InitTokenPool();
  InitMacroPool();
  InitCharTypeLookup();

  if (DoTests)
  {
    RunTests();
  }
  else
  {
    char *FilenameArg = ArgValues[2];
    u8 ModeArg = ArgValues[1][0];
    switch (ModeArg)
    {
      case 'c':
      {
        char *FilenameNoExt = (char *)NewTempBuffer(TEMP_BUFFER_SIZE);
        strcpy_s(FilenameNoExt, TEMP_BUFFER_SIZE, FilenameArg);

        int CompileType = COMPILE_TYPE_MAP;
        if (!strcmp(FilenameNoExt, "effects"))
          CompileType = COMPILE_TYPE_EFFECT;
        if (!strcmp(FilenameNoExt, "magic"))
          CompileType = COMPILE_TYPE_MAGIC;
        if (!strcmp(FilenameNoExt, "startup"))
          CompileType = COMPILE_TYPE_STARTUP;

        if (!CompileGuy.IsQuiet)
        {
          Log("vcc v.04.Jun.98 Copyright (C)1997 BJ Eirich \n");
        }

        PreStartupFiles(FilenameNoExt);
        FirstPass();
        PostStartupFiles();

        Compile(CompileType);

        switch (CompileType)
        {
          case COMPILE_TYPE_MAP: WriteOutput(FilenameNoExt); break;
          case COMPILE_TYPE_EFFECT: WriteEffectOutput(); break;
          case COMPILE_TYPE_MAGIC: WriteMagicOutput(); break;
          case COMPILE_TYPE_STARTUP: WriteScriptOutput(); break;
        }

        break;
      }

      case 'a': // disassemble
      case 'd': // decompile
      case 'v': // validate script offset table
      case 'x': // extract compiled script from map
      {
        decomp_mode Mode = ModeArg == 'a' ? DISASSEMBLE : DECOMPILE;

        buffer *Input = 0;
        buffer Output;

        char InputFilename[1024];
        u64 L = strlen(FilenameArg);
        strcpy_s(InputFilename, 1024, FilenameArg);
        if (L > 9 && !strcmp(".compiled", FilenameArg + (L - 9)))
        {
          Input = LoadEntireFile(InputFilename);
        }
        else
        {
          if (L > 4 && !strcmp(".map", FilenameArg + (L - 4))) {}
          else if (L > 4 && !strcmp(".vcs", FilenameArg + (L - 4)))
          {
          }
          else
            strcpy_s(InputFilename + L, 1024, ".map");

          u64 IL = strlen(InputFilename);

          if (Exist(InputFilename))
          {
            Input = LoadEntireFile(InputFilename);

            if (IL > 4 && !strcmp(".map", InputFilename + (IL - 4)))
            {
              v1map_header *MapHeader = (v1map_header *)Input->Data;
              u16 MapWidth = MapHeader->Width;
              u16 MapHeight = MapHeader->Height;
              DebugLog(LOW, "Map Size: %dx%d\n", MapWidth, MapHeight);
              Input->C = Input->Data + sizeof(v1map_header) +
                         (MapWidth * MapHeight * 5) + 7956;
              u32 NumEntities = Input->GetD();
              Input->C += sizeof(v1entity) * NumEntities;
              u8 NumMovementScripts = *Input->C++;
              u32 MovementScriptBufferSize = Input->GetD();
              Input->C += (NumMovementScripts * 4) + MovementScriptBufferSize;

              // NOTE(aen): Scripts begin here.
              u64 MapSize = Input->C - Input->Data;
              DebugLog(LOW, "MapSize %lld\n", MapSize);
              DebugLog(LOW, "Length Before: %lld\n", Input->Length);
              Input->Length -= MapSize;
              DebugLog(LOW, "Length After: %lld\n", Input->Length);
              Input->Data = Input->C;
            }

            if (ModeArg == 'v')
            {
              u32 NumScripts = Input->GetD();
              Log("Validating %d script offsets...\n", NumScripts);
              u8 *ScriptBase = Input->C + (NumScripts * 4);
              u32 *Offset = (u32 *)Input->C;
              u32 *OffsetBase = Offset;
              if (*Offset)
                Log("  Script index 0 must point to offset 0...FAIL. Points to "
                    "%d\n",
                    *Offset);
              Offset++;
              b64 HasFailed = false;
              for (u64 Index = 1; Index < NumScripts; Index++)
              {
                u8 *Head = ScriptBase + *Offset++;
                if (Head[-1] != 0xff)
                {
                  HasFailed = true;
                  u8 *P = Head - 1;
                  while (P != ScriptBase && *P != 0xff)
                  {
                    P--;
                  }
                  Log("  Script index %d...", Index);
                  Log("FAIL. Found 0x%x (%d). ", Head[-1], Head[-1]);
                  if (P == ScriptBase)
                  {
                    Log("    No preeding 0xff\n");
                  }
                  else
                  {
                    u64 NearestEndByte = Head - (P + 1);
                    Log("    0xff found %d bytes earlier\n", NearestEndByte);
                    // // NOTE(aen): Shift everything back. Temp theory
                    // // validation...
                    // for (u64 i = Index; i < NumScripts; i++)
                    // {
                    //   OffsetBase[i] -= (u32)NearestEndByte;
                    // }
                  }
                }
              }
              if (!HasFailed)
                Log("OK\n");
            }
            else if (ModeArg == 'x')
            {
              char NoExt[1024];
              strcpy_s(NoExt, 1024, FilenameArg);
              u64 L2 = strlen(NoExt);
              if (L2 > 4 && !strcmp(".map", NoExt + (L2 - 4)))
              {
                NoExt[L2 - 4] = 0;
              }

              char TempTable[1024];
              sprintf_s(TempTable, 1024, "%s.table", NoExt);
              char TempCompiled[1024];
              sprintf_s(TempCompiled, 1024, "%s.compiled", NoExt);

              Log("Extracting compiled script from %s...\n", FilenameArg);

              Log("Generating %s...\n", TempTable);
              FILE *File;
              fopen_s(&File, TempTable, "wb+");
              u8 *TableStart = Input->C;
              u32 NumScripts = Input->GetD();
              u64 OffsetTableSize = 4 + (NumScripts * 4);
              fwrite(TableStart, 1, OffsetTableSize, File);
              fclose(File);
              Input->C = TableStart + OffsetTableSize;
              Input->Length -= OffsetTableSize;

              Log("Generating %s...\n", TempCompiled);
              fopen_s(&File, TempCompiled, "wb+");
              fwrite(Input->C, 1, Input->Length, File);
              fclose(File);
              Log("OK\n");
            }
          }
          else
          {
            Fail("File not found: %s\n", Output.Data);
          }
        }

        if (Input)
        {
          Decompile(Input, &Output, PRODUCTION_MAX_TOKENS, Mode);
        }

        Log("%.*s", Output.Length, Output.Data);

        break;
      }

      case 'p':
      {
        parser Parser;
        Parser.Load(FilenameArg);

        token_list TLA;
        TLA.SetMaxTokens(PRODUCTION_MAX_TOKENS);
        Parser.ToTokenList(&TLA);

        macro_list ML;
        token_list TLB;
        TLB.SetMaxTokens(PRODUCTION_MAX_TOKENS);
        ParseMacros(&Parser, &TLA, &TLB, &ML);

        token_list TLC;
        TLC.SetMaxTokens(PRODUCTION_MAX_TOKENS);
        ExpandMacros(&ML, &TLB, &TLC);

        u8 *Dump = (u8 *)NewTempBuffer(MemLeft(&TempPool));
        TLC.Minify(Dump);

        Log("%s", Dump);

        break;
      }
    }
  }

  return 0;
}