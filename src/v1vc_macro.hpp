#ifndef V1VC_MACRO_HPP
#define V1VC_MACRO_HPP

#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"

struct Macro_t
{
  Token_t *Token = 0;
  struct TokenList_t ParamList;
  TokenList_t Expansion;

  void ParseFrom(TokenList_t *TokenList); // From current iteration index
  void Debug();
};

#define MACRO_SLAB_SIZE 1000
bool64 IsMacroSlabReady = false;
void InitMacroSlab();
void FreeMacroSlab();
Macro_t *NewMacro();
u64 MacroSlabResidents = 0;

struct MacroList_t
{
  Macro_t **Data = 0;
  u64 MaxMacros = 0;
  u64 NumMacros = 0;

  MacroList_t(u64 N = MACRO_SLAB_SIZE);
  ~MacroList_t();

  Macro_t *Get(u64 MacroIndex);

  void Reset(u64 N = MACRO_SLAB_SIZE);
  void Debug();

  void AddMacro(Macro_t *Macro);
};

// NOTE(aen): Builds a new token list with all macro definitions removed.
// Seems easier to expand macros in that new list.
// TODO(aen): Parent parser is currently only used for its path for #includes.
// Pass the path directly? #includes from in-memory parsing should prolly be
// relative to the CWD.
void ParseMacros(
    Parser_t *Parent,
    TokenList_t *In,
    TokenList_t *Out,
    MacroList_t *MacroList);
void ExpandMacros(MacroList_t *MacroList, TokenList_t *In, TokenList_t *Out);

#endif // V1VC_MACRO_HPP
