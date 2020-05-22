#ifndef COMPILE_HPP
#define COMPILE_HPP

#include "types.hpp"
#include <stdbool.h>

#define WORKING_MEMORY_TOTAL_SIZE (1024 * 1024)
#define WORKING_MEMORY_BLOCK_SIZE (1024 * 128)

#define COMPILE_TYPE_MAP 0
#define COMPILE_TYPE_STARTUP 1
#define COMPILE_TYPE_EFFECT 2
#define COMPILE_TYPE_MAGIC 3

struct label // goto labels
{
  char Identifier[40];
  u8 *Position;
};

// Character types

#define LETTER 1
#define DIGIT 2
#define SPECIAL 3

// Token types

#define IDENTIFIER 1
#define DIGIT 2
#define CONTROL 3
#define RESERVED 4
#define FUNCTION 5
#define VAR0 5
#define VAR1 6
#define VAR2 7

// NOTE(aen): An in-memory instance of the compiler.
struct compile_guy
{
  char *BasePath = 0;

  b64 IsQuiet = 0;
  b64 IsVerbose = 0;

  struct label Labels[200] = {};
  struct label Gotos[200] = {};

  char Token[2048] = {};
  char LastToken[2048] = {};
  u64 TokenType = 0;
  u64 TokenSubType = 0;

  // NOTE(aen): Single-allocation for everything
  void *WorkingMemory = 0;

  // NOTE(aen): These all point to location within working memory
  u8 *Data = 0;
  u64 Length = 0;
  u8 *C = 0;
  u8 *DataPreproc = 0;

  u8 *GeneratedCode = 0;
  u8 *GeneratedCodeLocation = 0;

  u8 *NumArgsLocation = 0;
  u32 ScriptOffsetTable[1024] = {};
  u64 NumScripts = 0;
  u64 NumLines = 0;
  u64 NumLabels = 0;
  u64 NumGotos = 0;

  b64 IsInEvent = 0;
  b64 IsExternalGarbage = 0;          // TODO(aen): Is this really what it is?
  const char *TopLevelScriptType = 0; // NOTE(aen): EFFECT, SCRIPT, EVENT
  u64 LibraryFunctionIndex = 0;
};

compile_guy CompileGuy;

// int GlobalNumScripts;
// unsigned int GlobalScriptOffsetTable[1024];
// char token[2048];
u8 CharTypeLookup[256];
void InitCharTypeLookup();

b64 IsCharType(u64 C, u64 Type);
void err(const char *str);
void EmitOperand();
void ProcessGoto();
void EmitD(u64 w);
void EmitC(u64 c);
void Expect(const char *str);
b64 NextIs(const char *str);
void GetString();
void EmitString(const char *str);
void CompileToBuffer(
    u64 Type,
    const char *Input,
    u8 *Output,
    u64 OutputLength,
    u64 *GeneratedByteCount);
void Compile(u64 Type, u8 *Input = 0, u8 *Output = 0);
void ProcessFor();
void ProcessSwitch();
void ProcessWhile();
void ProcessVar0Assign();
void ProcessVar1Assign();
void ProcessVar2Assign();
void ProcessIf();

#endif // COMPILE_HPP