#ifndef V1VC_TOKEN_HPP
#define V1VC_TOKEN_HPP

#include "mem.hpp"
#include "types.hpp"

#define PRODUCTION_MAX_TOKENS 30000

enum TokenType_e
{
  TT_UNDEFINED,
  TT_SYMBOL,
  TT_NUMBER,
  TT_STRING,
  TT_IDENT,
};

struct token
{
  u64 Line = 0;
  u64 Column = 0;
  u64 Start = 0;
  u64 End = 0;
  u64 Length = 0;
  TokenType_e Type = TT_UNDEFINED;
  char *Text = 0; // NOTE(aen): Points directly into loaded file memory

  void Debug();

  b64 IsNumber();
  b64 IsIdent();
  b64 IsMatch(token *Token);
  b64 IsMatch(char *S, u64 SLen);
  void ToString(char *Output, u64 OutputLength, u64 MaxTextLength = 20);
};

#define DEFAULT_NUM_TOKENS_PER_LIST 100

struct token_list
{
  token *Data = 0;
  u64 MaxTokens = 0;
  u64 NumTokens = 0;
  u64 Index = 0;

  token *Get(u64 TokenIndex);

  void Reset();
  void SetMaxTokens(u64 N = DEFAULT_NUM_TOKENS_PER_LIST);
  void Debug();

  token *AddToken(token *Token);
  token *AddToken(char *Text, u64 L, TokenType_e T, u64 S, u64 E);
  void NextToken(s64 Delta = 1);
  token *AtToken();
  b64 AtEnd();
  b64 IsToken(char C);
  b64 IsToken(char *Text, u64 L);
  b64 IsIdent();
  b64 IsNumber();
  void ExpectToken(char C, char Backup = 0);
  void ExpectToken(char *Text, u64 L);
  void ExpectTokenType(TokenType_e Type);

  // NOTE(aen): DryRun is for calculating needed output buffer size
  u64 Minify(u8 *Output, b64 ForceSpaces = false, b64 DryRun = false);
};

mem_pool TokenPool;
void InitTokenPool();
token *NewToken();

#endif // V1VC_TOKEN_HPP