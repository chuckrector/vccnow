#include "decompile.cpp"
#include "preproc_cmdline.cpp"
#include "vcc_cmdline.cpp"

#include "compile.cpp"
#include "funclib.cpp"
#include "libfuncs.cpp"
#include "log.cpp"
#include "nichgvc.cpp"
#include "preproc.cpp"
#include "ricvc.cpp"
#include "runtests.cpp"
#include "types.cpp"
#include "util.cpp"
#include "v1vc_macro.cpp"
#include "v1vc_parser.cpp"
#include "v1vc_token.cpp"
#include "vcc.cpp"

// #include <stdio.h>
#include <stdlib.h>
// #include <string.h>

#define BUILD 1

int
main(int argc, char *argv[])
{
  InitBufferSlab();
  InitStringHeap();
  InitTokenSlab();
  InitMacroSlab();
  InitCharTypeLookup(); // TODO(aen): Fail with a more intelligent error message
                        // when attempting to compile without this
  if (argc == 2 && argv[1][0] == 't')
  {
    RunTests();
  }
  else if (argc != 3)
  {
    Log("vccnow build %d\n", BUILD);
    Log("usage\n");
    Log("  preprocess: vccnow p <file.vc>\n");
    Log("     compile: vccnow c <file.vc>\n");
    Log("   decompile: vccnow d <file.map>\n");
    Log("       tests: vccnow t\n");
    exit(-1);
  }

  char *sub_argv[] = {argv[0], argv[2]};
  switch (argv[1][0])
  {
    case 'c': vcc_cmdline(2, sub_argv); break;
    case 'd': decompile_cmdline(2, sub_argv); break;
    case 'p': preproc_cmdline(2, sub_argv); break;
  }
  return 0;
}