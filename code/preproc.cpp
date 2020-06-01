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

#define LETTER 1
#define DIGIT 2
#define SPECIAL 3

void
FirstPass()
{
  parser Parser(NewBuffer(CompileGuy.Data, CompileGuy.Length));
  Parser.CalcPath(CompileGuy.BasePath); // NOTE(aen): For in-mem #include error
  token_list TokenList;
  TokenList.SetMaxTokens(PRODUCTION_MAX_TOKENS);
  Parser.ToTokenList(&TokenList);

  macro_list MacroList;
  token_list TokenListWithoutMacros;
  TokenListWithoutMacros.SetMaxTokens(PRODUCTION_MAX_TOKENS);
  ParseMacros(&Parser, &TokenList, &TokenListWithoutMacros, &MacroList);

  if (CompileGuy.IsVerbose)
    Log("Expanding macros...\n");
  token_list TokenListExpanded;
  TokenListExpanded.SetMaxTokens(PRODUCTION_MAX_TOKENS);
  ExpandMacros(&MacroList, &TokenListWithoutMacros, &TokenListExpanded);
  TokenListExpanded.Minify(CompileGuy.DataPreproc);

  if (CompileGuy.IsVerbose)
    printf("Preprocessing completed successfully\n");
  if (CompileGuy.IsVerbose)
    printf("Compiling scripts... \n");

  // Log("%s\n", CompileGuy.DataPreproc);
}
