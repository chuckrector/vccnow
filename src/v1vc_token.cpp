#include "v1vc_token.hpp"
#include "log.hpp"
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------

Token_t *TokenSlab = 0;
void
InitTokenSlab()
{
  if (IsTokenSlabReady)
    return;
  TokenSlab = new Token_t[TOKEN_SLAB_SIZE];
  IsTokenSlabReady = true;
}
void
FreeTokenSlab()
{
  delete[] TokenSlab;
}

// NOTE(aen): Never dealloc. Use what we got or boom
Token_t *
NewToken()
{
  if (!IsTokenSlabReady)
    Fail("Error: Must call InitTokenSlab before using NewToken\n");
  if (TokenSlabResidents >= TOKEN_SLAB_SIZE)
    Fail("Too many heap tokens! Over %d, exiting...\n", TOKEN_SLAB_SIZE);
  // Log("<<<NewToken %d>>>\n", TokenSlabResidents);
  return TokenSlab + TokenSlabResidents++;
}

// -----------------------------------------------------------------------------

void
DebugTokenType(TokenType_e Type)
{
  switch (Type)
  {
    case TT_SYMBOL: Log("TT_SYMBOL"); break;
    case TT_NUMBER: Log("TT_NUMBER"); break;
    case TT_STRING: Log("TT_STRING"); break;
    case TT_IDENT: Log("TT_IDENT"); break;
    default: Log("Uknown");
  }
}

// -----------------------------------------------------------------------------

bool64
Token_t::IsIdent()
{
  return Type == TT_IDENT;
}
bool64
Token_t::IsNumber()
{
  return Type == TT_NUMBER;
}
bool64
Token_t::IsMatch(const char *S, u64 SLength)
{
  return SLength == Length && !memcmp(S, Text, Length);
}
bool64
Token_t::IsMatch(Token_t *Token)
{
  return Type == Token->Type && Length == Token->Length &&
         !memcmp(Text, Token->Text, Length);
}

void
Token_t::ToString(char *Output, u64 OutputLength, u64 MaxTextLength)
{
  u64 L = Length;
  char *Ellipsis = "";
  if (L > MaxTextLength)
  {
    L = MaxTextLength;
    Ellipsis = "[...]";
  }
  snprintf(TempBuffer, TEMP_BUFFER_SIZE, "%s%s", Text, Ellipsis);
  snprintf(
      Output,
      OutputLength,
      "%s '%c' (%d)",
      TempBuffer,
      TempBuffer[0],
      TempBuffer[0]);
}

void
Token_t::Debug()
{
  if (Length < 0)
    Fail("Token length is negative! %d Exiting...\n", Length);
  Log("Token@%d:%d: %.*s (%d)\n", Line, Column, Length, Text, Length);
}

// -----------------------------------------------------------------------------

// NOTE(aen): Alloc/free pointer list only. Fine cz underlying tokens in slab
TokenList_t::TokenList_t(u64 N) { Reset(N); }
TokenList_t::~TokenList_t() { delete[] Data; }
Token_t *
TokenList_t::Get(u64 TokenIndex)
{
  return Data[TokenIndex];
}

void
TokenList_t::Reset(u64 N)
{
  if (MaxTokens != N)
  {
    if (Data)
      delete[] Data;
    Data = new Token_t *[MaxTokens = N];
  }
  NumTokens = 0;
  Index = 0;
}

void
TokenList_t::Debug()
{
  for (u64 T = 0; T < NumTokens; T++)
  {
    Log("%04d ", T);
    Get(T)->Debug();
  }
}

Token_t *
TokenList_t::AddToken(Token_t *Token)
{
  if (NumTokens >= MaxTokens)
    Fail("Too many tokens! Over %d, exiting...\n", MaxTokens);
  return Data[NumTokens++] = Token;
}

// NOTE(aen): Mostly for tests
Token_t *
TokenList_t::AddToken(char *Text, u64 L, TokenType_e T, u64 S, u64 E)
{
  Token_t *Token = NewToken();
  Token->Text = Text;
  Token->Length = L;
  Token->Type = T;
  Token->Start = S;
  Token->End = E;
  return this->AddToken(Token);
}

void
TokenList_t::NextToken(s64 Delta)
{
  Index += Delta;
}
Token_t *
TokenList_t::AtToken()
{
  return Data[Index];
}
bool64
TokenList_t::AtEnd()
{
  return Index >= NumTokens;
}
bool64
TokenList_t::IsToken(char Char)
{
  return AtToken()->Text[0] == Char;
}

bool64
TokenList_t::IsToken(const char *CheckText, u64 Length)
{
  return Length == AtToken()->Length &&
         !memcmp(CheckText, AtToken()->Text, Length);
}

bool64
TokenList_t::IsIdent()
{
  return AtToken()->IsIdent();
}
bool64
TokenList_t::IsNumber()
{
  return AtToken()->IsNumber();
}

// NOTE(aen): Backup is mainly for [] versus (). V1 uses them interchangeably.
void
TokenList_t::ExpectToken(char Char, char Backup)
{
  Token_t *Token = AtToken();
  if (Token->Text[0] == Char || (Backup && Token->Text[0] == Backup))
  {
    NextToken();
    return;
  }

  Log("Expected '%c' but got ", Char);
  Token->Debug();
  Fail("");
  // exit(-1);
}

void
TokenList_t::ExpectToken(const char *CheckText, u64 Length)
{
  Token_t *Token = AtToken();
  if (Length == Token->Length && !memcmp(Token->Text, CheckText, Length))
  {
    NextToken();
    return;
  }

  Log("Expected '%s' but got ", CheckText);
  Token->Debug();
  exit(-1);
}

void
TokenList_t::ExpectTokenType(TokenType_e Type)
{
  Token_t *Token = AtToken();
  if (Token->Type == Type)
  {
    NextToken();
    return;
  }

  Log("Expected token type of ");
  DebugTokenType(Type);
  Log(" but got ");
  DebugTokenType(Token->Type);
  Token->Debug();
  exit(-1);
}

u64
TokenList_t::Minify(u8 *Out, bool64 ForceSpaces)
{
  u8 *Start = Out;

  while (!AtEnd())
  {
    Token_t *Token = AtToken();
    memcpy(Out, Token->Text, Token->Length);
    if (Token->IsIdent())
    {
      for (u8 *P = Out, *PEnd = P + Token->Length; P < PEnd; P++)
        if (*P >= 'A' && *P <= 'Z')
          *P += 32;
    }
    Out += Token->Length;

    TokenType_e PrevType = Token->Type;
    NextToken();
    bool64 IsFlushIdentifiers = !AtEnd() && IsIdent() && PrevType == TT_IDENT;
    bool64 IsFlushIdentNum = !AtEnd() && IsNumber() && PrevType == TT_IDENT;
    bool64 IsFlushNumIdent = !AtEnd() && IsIdent() && PrevType == TT_NUMBER;
    if (!AtEnd() && (IsFlushIdentifiers || IsFlushIdentNum || IsFlushNumIdent ||
                     ForceSpaces))
      *Out++ = ' ';
  }

  *Out++ = 0;

  return Out - Start;
}
