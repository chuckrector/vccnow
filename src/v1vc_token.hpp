#ifndef V1VC_TOKEN_HPP
#define V1VC_TOKEN_HPP

#include "types.hpp"
#include <string.h>

#define TOKEN_SLAB_SIZE 20000
u64 TokenSlabResidents = 0;

enum TokenType_e
{
  TT_UNDEFINED,
  TT_SYMBOL,
  TT_NUMBER,
  TT_STRING,
  TT_IDENT,
};

struct Token_t
{
  u64 Line = 0;
  u64 Column = 0;
  u64 Start = 0;
  u64 End = 0;
  u64 Length = 0;
  TokenType_e Type = TT_UNDEFINED;
  char *Text = 0; // NOTE(aen): Pou64s directly u64o file

  void Debug();

  bool64 IsNumber();
  bool64 IsIdent();
  bool64 IsMatch(Token_t *Token);
  bool64 IsMatch(const char *S, u64 SLen);
  void ToString(char *Output, u64 OutputLength, u64 MaxTextLength = 20);
};

struct TokenList_t
{
  Token_t **Data = 0;
  u64 MaxTokens = 0;
  u64 NumTokens = 0;
  u64 Index = 0;

  TokenList_t(u64 N = TOKEN_SLAB_SIZE);
  ~TokenList_t();

  Token_t *Get(u64 TokenIndex);

  void Reset(u64 N = TOKEN_SLAB_SIZE);
  void Debug();

  Token_t *AddToken(Token_t *Token);
  Token_t *AddToken(char *Text, u64 L, TokenType_e T, u64 S, u64 E);
  void NextToken(s64 Delta = 1);
  Token_t *AtToken();
  bool64 AtEnd();
  bool64 IsToken(char C);
  bool64 IsToken(const char *Text, u64 L);
  bool64 IsIdent();
  bool64 IsNumber();
  void ExpectToken(char C, char Backup = 0);
  void ExpectToken(const char *Text, u64 L);
  void ExpectTokenType(TokenType_e Type);

  u64 Minify(u8 *Output, bool64 ForceSpaces = false);
};

void InitTokenSlab();
void FreeTokenSlab();
Token_t *NewToken();
bool64 IsTokenSlabReady = false;

#endif // V1VC_TOKEN_HPP