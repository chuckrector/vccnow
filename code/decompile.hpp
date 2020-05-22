#ifndef DECOMPILE_HPP
#define DECOMPILE_HPP

#include "buffer.hpp"
#include "types.hpp"
#include "util.hpp"
#include "v1vc_token.hpp"

// NOTE(aen): I don't remember why I duplicated all these op-code constants and
// prefixed them all with VC_ and used them instead. Rather than multiple defs
// for ENDSCRIPT and OP_END, there's just VC_END.

#define VC_END 255

// Single-byte opcode descriptors

#define VC_EXEC 1
#define VC_VAR0_ASSIGN 2
#define VC_VAR1_ASSIGN 3
#define VC_VAR2_ASSIGN 4
#define VC_GENERAL_IF 5
#define VC_ELSE 6
#define VC_GOTO 7
#define VC_FOR_LOOP0 8
#define VC_FOR_LOOP1 9
#define VC_SWITCH 10
#define VC_CASE 11

// Single-byte operand descriptors

#define VC_OP_IMMEDIATE 1
#define VC_OP_VAR0 2
#define VC_OP_VAR1 3
#define VC_OP_VAR2 4
#define VC_OP_GROUP 5

// Single-byte IF handler parameters

#define VC_ZERO 0
#define VC_NONZERO 1
#define VC_EQUALTO 2
#define VC_NOTEQUAL 3
#define VC_GREATERTHAN 4
#define VC_GREATERTHANOREQUAL 5
#define VC_LESSTHAN 6
#define VC_LESSTHANOREQUAL 7

// Single byte assignment descriptors

#define VC_SET 1
#define VC_INCREMENT 2
#define VC_DECREMENT 3
#define VC_INCSET 4
#define VC_DECSET 5

// Operand combination descriptors
#define VC_ADD 1
#define VC_SUB 2
#define VC_MULT 3
#define VC_DIV 4
#define VC_MOD 5

// Token types

#define VC_TT_IDENT 1
#define VC_TT_DIGIT 2
#define VC_TT_CONTROL 3
#define VC_TT_RESERVED 4
#define VC_TT_FUNCTION 5
#define VC_TT_VAR0 5
#define VC_TT_VAR1 6
#define VC_TT_VAR2 7

enum decomp_mode
{
  DISASSEMBLE,
  DECOMPILE
};

void Decompile(buffer *Input, buffer *Output, u64 MaxTokens, decomp_mode Mode);

struct decompiler
{
  u8 *Data = 0;
  u8 *DataEnd = 0;
  u64 DataSize = 0;
  u8 *ScriptBase = 0; // NOTE(aen): Offset after # scripts & offset table
  u64 NumScripts = 0;
  u32 *ScriptOffsetTable = 0;
  u64 *IfRefs = 0;
  u64 NumIfRefs = 0;
  u64 *AddressRefs = 0; // NOTE(aen): All addresses referenced by IF, GOTO, etc.
  u64 NumAddressRefs = 0;
  u64 *GotoRefs = 0;
  u64 NumGotoRefs = 0;
  decomp_mode Mode = DECOMPILE;

  char DisMarker = 0;
  char PrevDisMarker = 0;
  u32 DisAddress = 0xffffffff;
  char *DisByteCodesP = 0;
  char DisTemp[TEMP_BUFFER_SIZE];
  char DisByteCodes[TEMP_BUFFER_SIZE];
  char DisComments[TEMP_BUFFER_SIZE];
  void DisSaveAddress(char Marker = ' ');
  void DisLogComments(const char *Format, ...);
  void DisFlush();
  void DisD(u32 Value);
  void DisW(u16 Value);
  void DisC(u8 Value);
  void DisRefCheck();

  // NOTE(aen): Reset false for each event so that we can keep going and
  // gathering data.
  b64 ParseFailure = false;

  u8 *C = 0; // NOTE(aen): Current location in Data
  u64 CurrentScriptIndex = 0;
  u8 *CurrentScriptBase = 0;
  u8 *CurrentScriptEnd = 0;
  u64 CurrentScriptLength = 0;

  token_list TokenList;
  token *AddToken(char *Text, u64 L, TokenType_e Type);
  token *AddToken(char *Text, TokenType_e Type);
  token *AddTokenD(u32 Value);
  token *AddIdent(char *Text, u64 Length);
  token *AddIdent(char *Text);
  token *AddSymbol(char *Text);
  token *AddComment(char *Format, ...);

  void Init(buffer *Buffer, u64 MaxTokens, decomp_mode M);
  void Load(const char *Filename);
  void ToTokenList(token_list *TokenList);
  void Parse();
  void ParseBlock(u8 *End = 0, b64 EmitReturns = true);
  void ParseEvent(u64 ScriptIndex, u8 *RetryAddress = 0);
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
  void ParseExpression();
  void ParseGoto(b64 EmitDecompilation = true);
  void ParseFor0();
  void ParseFor1();
  void ParseSwitch();

  u32 GetD();
  u16 GetW();
  u32 PeekD();
  u16 PeekW();
};

#endif // DECOMPILE_HPP