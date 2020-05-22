#ifndef V1VC_MACRO_HPP
#define V1VC_MACRO_HPP

#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"

struct macro
{
  token *Token = 0;
  token_list ParamList;
  token_list Expansion; // NOTE(aen): Must call SetMaxTokens() when needed

  void ParseFrom(token_list *TokenList); // From current iteration index
  void Debug();
};

mem_pool MacroPool;
void InitMacroPool();
macro *NewMacro();

#define DEFAULT_NUM_MACROS_PER_LIST 1000

struct macro_list
{
  macro *Data = 0;
  u64 MaxMacros = 0;
  u64 NumMacros = 0;

  macro *Get(u64 MacroIndex);

  void Reset();
  void SetMaxMacros(u64 N);
  void Debug();

  void AddMacro(macro *Macro);
};

// NOTE(aen): Builds a new token list with all macro definitions removed.
// Seems easier to expand macros in that new list.
// TODO(aen): Parent parser is currently only used for its path for #includes.
// Pass the path directly? #includes from in-memory parsing should prolly be
// relative to the CWD.
void ParseMacros(
    parser *Parent,
    token_list *In,
    token_list *Out,
    macro_list *MacroList);
void ExpandMacros(macro_list *MacroList, token_list *In, token_list *Out);

#endif // V1VC_MACRO_HPP
