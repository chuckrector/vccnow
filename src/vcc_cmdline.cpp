#include "vcc_cmdline.hpp"
#include "compile.hpp"
#include "funclib.hpp"
#include "libfuncs.hpp"
#include "log.hpp"
#include "nichgvc.hpp"
#include "preproc.hpp"
#include "ricvc.hpp"
#include "types.hpp"
#include "util.hpp"
#include "v1vc_macro.hpp"
#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"
#include "vcc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
vcc_cmdline(int argc, char *argv[])
{
  InitBufferSlab();
  InitTokenSlab();
  InitMacroSlab();
  InitCharTypeLookup();
  // RunTests();

  char *FilenameNoExt = 0;
  int CompileType = 0;

  switch (argc)
  {
    case 1:
    {
      printf("vcc v.04.Jun.98 Copyright (C)1997 BJ Eirich \n");
      printf("Usage: vcc <vc file> [flag] \n");
      printf("Where [flag] is one of the following: \n");
      printf("        q   Quiet mode - no output. \n");
      printf(
          "        v   Super-Verbose mode - more output than you want. \n \n");
      exit(-1);
    }
    case 3:
    {
      char *Flag = argv[2];
      if (Flag[0] == 'q')
        CompileGuy.IsQuiet = true;
      if (Flag[0] == 'v')
        CompileGuy.IsVerbose = 1;
    }
    case 2:
    {
      CompileType = COMPILE_TYPE_MAP;
      FilenameNoExt = new char[1024];
      strcpy_s(FilenameNoExt, 1024, argv[1]);
      // printf("FilenameNoExt %s\n", FilenameNoExt);
      // FilenameNoExt = argv[1];
      if (!strcmp(FilenameNoExt, "effects"))
        CompileType = COMPILE_TYPE_EFFECT;
      if (!strcmp(FilenameNoExt, "magic"))
        CompileType = COMPILE_TYPE_MAGIC;
      if (!strcmp(FilenameNoExt, "startup"))
        CompileType = COMPILE_TYPE_STARTUP;
      break;
    }
    default:
    {
      Fail("vcc: Too many parameters.\n");
    }
  }

  if (!CompileGuy.IsQuiet)
  {
    printf("vcc v.04.Jun.98 Copyright (C)1997 BJ Eirich \n");
  }

  PreStartupFiles(FilenameNoExt);
  FirstPass();
  PostStartupFiles();
  InitCharTypeLookup();
  Compile(CompileType);
  switch (CompileType)
  {
    case COMPILE_TYPE_MAP: WriteOutput(FilenameNoExt); break;
    case COMPILE_TYPE_EFFECT: WriteEffectOutput(); break;
    case COMPILE_TYPE_MAGIC: WriteMagicOutput(); break;
    case COMPILE_TYPE_STARTUP: WriteScriptOutput(); break;
  }

  return 0;
}
