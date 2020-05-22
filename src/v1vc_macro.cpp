#include "v1vc_macro.hpp"
#include "compile.hpp"
#include "log.hpp"
#include "mem.hpp"
#include "util.hpp"
#include <stdlib.h>

// -----------------------------------------------------------------------------

void
InitMacroPool()
{
  Assert(!MacroPool.Base);
  InitMemPool(&MacroPool, PoolBase(MACRO_POOL_INDEX), POOL_SIZE);
}

// NOTE(aen): Never dealloc. Use what we got or boom
macro *
NewMacro()
{
  // Log("<<<NewMacro %d>>>\n", MacroPool.Residents);
  Assert(MacroPool.Base);
  macro *Result = NewItem(&MacroPool, macro);
  Result->ParamList.SetMaxTokens(4);
  Result->Expansion.SetMaxTokens(100);
  return Result;
}

// -----------------------------------------------------------------------------

void
macro::Debug()
{
  ParamList.Minify((u8 *)TempBuffer);
  Log("Macro@%d:%d: %.*s[%d] (%s) ",
      Token->Line,
      Token->Column,
      Token->Length,
      Token->Text,
      Token->Length,
      TempBuffer);
  Expansion.Minify((u8 *)TempBuffer);
  Log("-> %s\n", TempBuffer);
}

void
macro::ParseFrom(token_list *TokenList)
{
  TokenList->ExpectToken('#');
  TokenList->ExpectToken("define", 6);

  u64 Length = TokenList->AtToken()->Length;
  if (Length > 40)
    Fail("Macro name is too long. Over 40 chars, exiting...\n");

  Token = TokenList->AtToken();
  // Log("Macro name %.*s\n", Token->Length, Token->Text);

  TokenList->ExpectTokenType(TT_IDENT);

  if (CompileGuy.IsVerbose)
    Log("Found macro identifier %.*s\n", Token->Length, Token->Text);

  ParamList.NumTokens = 0;
  if (TokenList->IsToken('('))
  {
    TokenList->NextToken();

    while (!TokenList->AtEnd() && !TokenList->IsToken(')'))
    {
      // NOTE(aen): Saving token because Expect* moves to the next one.
      token *T = TokenList->AtToken();

      TokenList->ExpectTokenType(TT_IDENT);

      ParamList.AddToken(T);

      if (!TokenList->AtEnd() && TokenList->IsToken(','))
        TokenList->NextToken();
    }
    TokenList->ExpectToken(')');
  }

  TokenList->ExpectToken('@');

  while (!TokenList->AtEnd() && !TokenList->IsToken('@'))
  {
    Expansion.AddToken(TokenList->AtToken());
    TokenList->NextToken();
  }

  TokenList->ExpectToken('@');

  if (CompileGuy.IsVerbose)
  {
    Expansion.Minify((u8 *)TempBuffer);
    Log("Expands to: %s\n", TempBuffer);
  }

  if (CompileGuy.IsVerbose)
  {
    if (ParamList.NumTokens)
    {
      char *Output = TempBuffer;
      for (u64 i = 0; i < ParamList.NumTokens; i++)
      {
        memcpy(Output, ParamList.Get(i)->Text, ParamList.Get(i)->Length);
        Output += ParamList.Get(i)->Length;
      }
      *Output = 0;
      Log("Params: %s\n", TempBuffer);
    }
    else
    {
      Log("No params\n");
    }
  }
}

// -----------------------------------------------------------------------------

macro *
macro_list::Get(u64 MacroIndex)
{
  return Data + MacroIndex;
}

void
macro_list::Reset()
{
  NumMacros = 0;
}

void
macro_list::SetMaxMacros(u64 NewMaxMacros)
{
  if (MaxMacros != NewMaxMacros)
  {
    // NOTE(aen): Dumbly create a new list and copy items over. Don't bother
    // freeing anything. Bump allocation all the way until it's a problem.
    macro *OldData = Data;
    u64 OldMaxMacros = MaxMacros;
    Data = NewList(&MacroPool, MaxMacros = NewMaxMacros, macro);
    for (int N = 0; N < MaxMacros && N < OldMaxMacros; N++)
      Data[N] = OldData[N];
    DebugLog(
        MEDIUM,
        "MacroList.SetMaxMacros: Realloc from %lld to %lld\n",
        OldMaxMacros,
        MaxMacros);
  }
}

void
macro_list::Debug()
{
  for (u64 M = 0; M < NumMacros; M++)
  {
    Log("%04d ", M);
    Get(M)->Debug();
  }
}

void
macro_list::AddMacro(macro *Macro)
{
  if (!MaxMacros)
    SetMaxMacros(DEFAULT_NUM_MACROS_PER_LIST);
  if (NumMacros >= MaxMacros)
    Fail("Too many macros! Over %d, exiting...\n", MaxMacros);

  Data[NumMacros++] = *Macro;
}

// -----------------------------------------------------------------------------

// NOTE(aen): Parses from/to current indices, wherever those lie
void
ParseMacros(
    parser *Parent,
    token_list *In,
    token_list *Out,
    macro_list *MacroList)
{
  // Log("ParseMacros\n");
  while (!In->AtEnd())
  {
    u64 SaveTokenIndex = In->Index;
    // Log("SaveTokenIndex %lld\n", SaveTokenIndex);
    if (In->IsToken('#'))
    {
      In->NextToken();
      if (In->IsToken("define", 6))
      {
        // Log("#define\n");
        In->Index = SaveTokenIndex;
        macro *Macro = NewMacro();
        Macro->ParseFrom(In);
        MacroList->AddMacro(Macro);
      }
      else if (In->IsToken("include", 7))
      {
        In->NextToken();
        token *Token = In->AtToken();
        // Log("--> #include %.*s\n", Token->Length, Token->Text);
        In->NextToken();

        // TODO(aen): Set CWD on initial setup so all parsers have this
        // Log("Parent path %s\n", Parent->Path);
        if (!Parent->Path)
          Fail("Cannot #include from in-memory parser atm\n");
        sprintf_s(
            TempBuffer,
            TEMP_BUFFER_SIZE,
            "%s%.*s",
            Parent->Path,
            (int)Token->Length,
            Token->Text);

        parser P;
        // P.Path = Parent->Path;
        P.Load(TempBuffer);
        token_list TLA;
        TLA.SetMaxTokens(In->MaxTokens);
        P.ToTokenList(&TLA);
        // TLA.Debug();

        token_list TLB;
        TLB.SetMaxTokens(In->MaxTokens);
        macro_list ML;
        ParseMacros(Parent, &TLA, &TLB, &ML);

        for (u64 T = 0; T < TLB.NumTokens; T++)
          Out->AddToken(TLB.Get(T));
        for (u64 M = 0; M < ML.NumMacros; M++)
          MacroList->AddMacro(ML.Get(M));
      }
      else
      {
        Log("Unexpected directive: ");
        In->AtToken()->Debug();
        exit(-1);
      }
    }
    else
    {
      Out->AddToken(In->AtToken());
      In->NextToken();
    }
  }
}

void
ExpandMacros(macro_list *MacroList, token_list *In, token_list *Out)
{
  while (!In->AtEnd())
  {
    token *Token = In->AtToken();
    bool FoundMacro = false;

    // TODO(aen): Hash lookup
    for (u64 m = 0; m < MacroList->NumMacros; m++)
    {
      macro *Macro = MacroList->Get(m);
      bool IsMatch = Token->IsMatch(Macro->Token);
      if (Token->IsIdent() && IsMatch)
      {
        FoundMacro = true;

        // NOTE(aen): Don't add any more tokens to the output list until we
        // parse the full macro usage and know the substitution values.

        // Log("Found macro for expansion: ");
        // Token->Debug();
        In->NextToken();

        token_list *SubList =
            NewList(&TokenPool, Macro->ParamList.NumTokens, token_list);
        In->ExpectToken('(', '[');
        for (u64 p = 0; p < Macro->ParamList.NumTokens; p++)
        {
          token_list *SL = SubList + p;
          SL->SetMaxTokens(1000); // TODO(aen): Dynamify
          // NOTE(aen): Gobble param, even if it's a complex expression.
          while (!In->AtEnd() && !In->IsToken(')') && !In->IsToken(']') &&
                 !In->IsToken(','))
          {
            // Log("Expansion token: %.*s\n", In->AtToken()->Length,
            //     In->AtToken()->Text);
            SL->AddToken(In->AtToken());
            In->NextToken();
          }
          if (p < Macro->ParamList.NumTokens - 1)
            In->ExpectToken(',');
        }

        In->ExpectToken(')', ']');

        // NOTE(aen): Now add all the expansion tokens to the output list,
        // making substitutions where needed.
        Macro->Expansion.Index = 0;
        while (!Macro->Expansion.AtEnd())
        {
          token *TokenToWrite = Macro->Expansion.AtToken();

          s64 Match = -1;
          for (u64 P = 0; P < Macro->ParamList.NumTokens; P++)
          {
            token *T = Macro->ParamList.Get(P);
            if (TokenToWrite->IsMatch(T))
            {
              Match = P;
              break;
            }
          }

          if (Match >= 0)
          {
            token_list *SLP = SubList + Match;
            // NOTE(aen): Write out full expansion
            for (int X = 0; X < SLP->NumTokens; X++)
              Out->AddToken(SLP->Get(X));
          }
          else
          {
            Out->AddToken(TokenToWrite);
          }

          Macro->Expansion.NextToken();
        }

        break;
      }
    }

    if (!FoundMacro)
    {
      // NOTE(aen): Copy all non-macro tokens
      Out->AddToken(In->AtToken());
      In->NextToken();
    }
  }
}
