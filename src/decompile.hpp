#ifndef DECOMPILE_HPP
#define DECOMPILE_HPP

#include "types.hpp"
#include "v1vc_token.hpp"

void InitStringHeap();
void FreeStringHeap();
u8 *NewString(char *S, u64 L = 0);
bool64 IsStringHeapReady = false;

struct Decomp_t
{
  u8 *Data = 0;
  u8 *DataEnd = 0;
  u64 DataSize = 0;
  u8 *ScriptBase = 0; // NOTE(aen): Offset after # scripts & offset table
  u64 NumScripts = 0;
  u32 *ScriptOffsetTable = 0;
  u64 *IfOffsetTable = 0;
  u64 *GotoOffsetTable;

  u8 *C = 0; // NOTE(aen): Current location in Data
  u8 *CurrentScriptBase = 0;
  u8 *CurrentScriptEnd = 0;
  u64 CurrentScriptLength = 0;

  TokenList_t TokenList;
  Token_t *AddToken(char *Text, u64 L, TokenType_e Type);
  Token_t *AddToken(char *Text, TokenType_e Type);
  Token_t *AddTokenD(u32 Value);
  Token_t *AddIdent(char *Text, u64 Length);
  Token_t *AddIdent(char *Text);
  Token_t *AddSymbol(char *Text);

  void Init(Buffer_t *Buffer);
  void Load(const char *Filename);
  void ToTokenList(TokenList_t *TokenList);
  void Parse();
  void ParseBlock(u8 *End = 0, bool64 EmitReturns = true);
  void ParseEvent(u64 ScriptIndex);
  void Debug();
  u64 ExpectOpCode(u64 Code);

  char *ParseParam(u64 T1, char *P);
  void ParseLibFunc();
  void ParseOperand();
  void ParseVar0();
  void ParseVar1();
  void ParseVar2();
  void ParseVarAssignment();
  void ParseOperandPrimitive();
  void ParseString();
  void ParseIfTerm();
  void ParseIf();
  void ParseReturn();
  void NewParseExpression();
  bool64 ParseExpression();
  void ParseGoto();
  void ParseFor0();
  void ParseFor1();
  void ParseSwitch();

  u32 GetD();
  u16 GetW();
  u32 PeekD();
  u16 PeekW();
};

#endif // DECOMPILE_HPP