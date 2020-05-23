#include "v1vc_token.hpp"
#include "log.hpp"
#include "mem.hpp"
#include "util.hpp"
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------

void
InitTokenPool()
{
  Assert(!TokenPool.Base);
  InitMemPool(&TokenPool, PoolBase(TOKEN_POOL_INDEX), TOKEN_POOL_SIZE);
}

token *
NewToken()
{
  // Log("<<<NewToken %d>>>\n", TokenPool.Residents);
  Assert(TokenPool.Base);
  token *Result = NewItem(&TokenPool, token);
  return Result;
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

b64
token::IsIdent()
{
  return Type == TT_IDENT;
}
b64
token::IsNumber()
{
  return Type == TT_NUMBER;
}
b64
token::IsMatch(const char *S, u64 SLength)
{
  return SLength == Length && !memcmp(S, Text, Length);
}
b64
token::IsMatch(token *Token)
{
  return Type == Token->Type && Length == Token->Length &&
         !memcmp(Text, Token->Text, Length);
}

void
token::ToString(char *Output, u64 OutputLength, u64 MaxTextLength)
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
token::Debug()
{
  if (Length < 0)
    Fail("Token length is negative! %d Exiting...\n", Length);
  Log("Token@%d:%d: %.*s (%d)\n", Line, Column, Length, Text, Length);
}

// -----------------------------------------------------------------------------

token *
token_list::Get(u64 TokenIndex)
{
  return Data + TokenIndex;
}

void
token_list::Reset()
{
  NumTokens = 0;
  Index = 0;
}

void
token_list::SetMaxTokens(u64 NewMaxTokens)
{
  DebugLog(MEDIUM, "TokenList.SetMaxTokens %d\n", NewMaxTokens);
  if (MaxTokens != NewMaxTokens)
  {
    // NOTE(aen): Dumbly create a new list and copy items over. Don't bother
    // freeing anything. Bump allocation all the way until it's a problem.
    token *OldData = Data;
    u64 OldMaxTokens = MaxTokens;
    Data = NewList(&TokenPool, MaxTokens = NewMaxTokens, token);
    for (int N = 0; N < MaxTokens && N < OldMaxTokens; N++)
      Data[N] = OldData[N];
    DebugLog(
        MEDIUM,
        "TokenList.SetMaxTokens: Realloc from %lld to %lld\n",
        OldMaxTokens,
        MaxTokens);
  }
}

void
token_list::Debug()
{
  for (u64 T = 0; T < NumTokens; T++)
  {
    Log("%04d ", T);
    Get(T)->Debug();
  }
}

token *
token_list::AddToken(token *Token)
{
  if (!MaxTokens)
    SetMaxTokens(DEFAULT_NUM_TOKENS_PER_LIST);
  if (NumTokens >= MaxTokens)
    Fail("Too many tokens! Over %d, exiting...\n", MaxTokens);

  token *Result = Data + NumTokens++;

  // NOTE(aen): Text will point to original memory. Minification results can
  // be incorrect (e.g. event/*0*/{} markers) if folks adding tokens don't
  // allocate new memory for each new token's text. For tokens that point
  // directly into the memory for a file loaded from disk, no new allocation
  // is needed.
  *Result = *Token;

  return Result;
}

// NOTE(aen): Mostly for tests
token *
token_list::AddToken(char *Text, u64 L, TokenType_e T, u64 S, u64 E)
{
  token *Token = NewToken();
  Token->Text = Text;
  Token->Length = L;
  Token->Type = T;
  Token->Start = S;
  Token->End = E;
  return this->AddToken(Token);
}

void
token_list::NextToken(s64 Delta)
{
  Index += Delta;
}

token *
token_list::AtToken()
{
  return Data + Index;
}

b64
token_list::AtEnd()
{
  return Index >= NumTokens;
}

b64
token_list::IsToken(char Char)
{
  return AtToken()->Text[0] == Char;
}

b64
token_list::IsToken(const char *CheckText, u64 Length)
{
  return Length == AtToken()->Length &&
         !memcmp(CheckText, AtToken()->Text, Length);
}

b64
token_list::IsIdent()
{
  return AtToken()->IsIdent();
}

b64
token_list::IsNumber()
{
  return AtToken()->IsNumber();
}

// NOTE(aen): Backup is mainly for [] versus (). V1 uses them interchangeably.
void
token_list::ExpectToken(char Char, char Backup)
{
  token *Token = AtToken();
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
token_list::ExpectToken(const char *CheckText, u64 Length)
{
  token *Token = AtToken();
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
token_list::ExpectTokenType(TokenType_e Type)
{
  token *Token = AtToken();
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
token_list::Minify(u8 *Out, b64 ForceSpaces, b64 DryRun)
{
  u64 N = 0;

  // int outer = 0;
  while (!AtEnd())
  {
    // Log("Minify outer %d\n", outer++);
    token *Token = AtToken();
    // Token->Debug();
    if (!DryRun)
    {
      memcpy(Out + N, Token->Text, Token->Length);
      if (Token->IsIdent())
      {
        // int inner = 0;
        for (u8 *P = Out + N, *PEnd = P + Token->Length; P < PEnd; P++)
        {
          // Log("  inner %d\n", inner++);
          if (*P >= 'A' && *P <= 'Z')
            *P += 32;
        }
      }
    }
    N += Token->Length;

    TokenType_e PrevType = Token->Type;
    NextToken();
    b64 IsFlushIdentifiers = !AtEnd() && IsIdent() && PrevType == TT_IDENT;
    b64 IsFlushIdentNum = !AtEnd() && IsNumber() && PrevType == TT_IDENT;
    b64 IsFlushNumIdent = !AtEnd() && IsIdent() && PrevType == TT_NUMBER;
    if (!AtEnd() && (IsFlushIdentifiers || IsFlushIdentNum || IsFlushNumIdent ||
                     ForceSpaces))
    {
      if (!DryRun)
        Out[N] = ' ';
      N++;
    }
  }

  // if (!DryRun)
  //   Out[N] = 0;
  // N++;

  return N;
}
