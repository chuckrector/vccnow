#include "preproc.hpp"
#include "compile.hpp"
#include "log.hpp"
#include "types.hpp"
#include "util.hpp"
#include "v1vc_macro.hpp"
#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"
#include "vcc.hpp"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LETTER 1
#define DIGIT 2
#define SPECIAL 3

void
FirstPass()
{
  Parser_t Parser(NewBuffer(CompileGuy.Data, CompileGuy.Length));
  Parser.CalcPath(CompileGuy.BasePath); // NOTE(aen): For in-mem #include error
  TokenList_t TokenList;
  Parser.ToTokenList(&TokenList);

  MacroList_t MacroList;
  TokenList_t TokenListWithoutMacros;
  ParseMacros(&Parser, &TokenList, &TokenListWithoutMacros, &MacroList);

  if (CompileGuy.IsVerbose)
    Log("Expanding macros...\n");
  TokenList_t TokenListExpanded;
  ExpandMacros(&MacroList, &TokenListWithoutMacros, &TokenListExpanded);
  TokenListExpanded.Minify(CompileGuy.DataPreproc);

  if (CompileGuy.IsVerbose)
    printf("Preprocessing completed successfully\n");
  if (CompileGuy.IsVerbose)
    printf("Compiling scripts... \n");

  // Log("%s\n", CompileGuy.DataPreproc);
}
