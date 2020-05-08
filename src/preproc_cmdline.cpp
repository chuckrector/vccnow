#include "preproc_cmdline.hpp"
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

int
preproc_cmdline(int argc, char *argv[])
{
  InitBufferSlab();
  InitTokenSlab();
  InitMacroSlab();
  InitCharTypeLookup();

  if (argc != 2)
    Fail("Usage: preproc <file.vc>\n");
  const char *Filename = argv[1];

  Parser_t Parser;
  Parser.Load(Filename);
  TokenList_t TLA;
  Parser.ToTokenList(&TLA);

  MacroList_t ML;
  TokenList_t TLB;
  ParseMacros(&Parser, &TLA, &TLB, &ML);
  TokenList_t TLC;
  ExpandMacros(&ML, &TLB, &TLC);

  u8 *Dump = new u8[WORKING_MEMORY_BLOCK_SIZE];
  TLC.Minify(Dump);
  Log("%s", Dump);
  delete[] Dump;

  // Log("\n%s: %d{%d} tokens, %d{%d} macros\n",
  //     Filename,
  //     TLB.NumTokens,
  //     TokenSlabResidents,
  //     ML.NumMacros,
  //     MacroSlabResidents);
  return 0;
}
