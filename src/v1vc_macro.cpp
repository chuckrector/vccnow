#include "v1vc_macro.hpp"
#include "compile.hpp"
#include "log.hpp"
#include <stdlib.h>

// -----------------------------------------------------------------------------

Macro_t *MacroSlab;
void
InitMacroSlab()
{
  if (IsMacroSlabReady)
    return;
  MacroSlab = new Macro_t[MACRO_SLAB_SIZE];
  IsMacroSlabReady = true;
}
void
FreeMacroSlab()
{
  delete[] MacroSlab;
}

// NOTE(aen): Never dealloc. Use what we got or boom
Macro_t *
NewMacro()
{
  if (!IsMacroSlabReady)
    Fail("Error: Must call InitMacroSlab before using NewMacro\n");
  if (MacroSlabResidents >= MACRO_SLAB_SIZE)
    Fail("Too many slab macros! Over %d, exiting...\n", MACRO_SLAB_SIZE);
  // Log("<<<NewMacro %d>>>\n", MacroSlabResidents);
  return MacroSlab + MacroSlabResidents++;
}

// -----------------------------------------------------------------------------

void
Macro_t::Debug()
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
Macro_t::ParseFrom(TokenList_t *TokenList)
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
      Token_t *T = TokenList->AtToken();

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

// NOTE(aen): Alloc/free pointer list only. Fine cz underlying tokens in slab
MacroList_t::MacroList_t(u64 N) { Reset(N); }
MacroList_t::~MacroList_t() { delete[] Data; }
Macro_t *
MacroList_t::Get(u64 MacroIndex)
{
  return Data[MacroIndex];
}

void
MacroList_t::Reset(u64 N)
{
  if (MaxMacros != N)
  {
    if (Data)
      delete[] Data;
    Data = new Macro_t *[MaxMacros = N];
  }
  NumMacros = 0;
}

void
MacroList_t::Debug()
{
  for (u64 M = 0; M < NumMacros; M++)
  {
    Log("%04d ", M);
    Get(M)->Debug();
  }
}

void
MacroList_t::AddMacro(Macro_t *Macro)
{
  if (NumMacros >= MaxMacros)
    Fail("Too many macros! Over %d, exiting...\n", MaxMacros);
  Data[NumMacros++] = Macro;
}

// -----------------------------------------------------------------------------

// NOTE(aen): Parses from/to current indices, wherever those lie
void
ParseMacros(
    Parser_t *Parent,
    TokenList_t *In,
    TokenList_t *Out,
    MacroList_t *MacroList)
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
        Macro_t *Macro = NewMacro();
        Macro->ParseFrom(In);
        MacroList->AddMacro(Macro);
      }
      else if (In->IsToken("include", 7))
      {
        In->NextToken();
        Token_t *Token = In->AtToken();
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

        Parser_t P;
        // P.Path = Parent->Path;
        P.Load(TempBuffer);
        TokenList_t TLA;
        P.ToTokenList(&TLA);
        // TLA.Debug();

        TokenList_t TLB;
        MacroList_t ML;
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
ExpandMacros(MacroList_t *MacroList, TokenList_t *In, TokenList_t *Out)
{
  while (!In->AtEnd())
  {
    Token_t *Token = In->AtToken();
    bool FoundMacro = false;

    // TODO(aen): Hash lookup
    for (u64 m = 0; m < MacroList->NumMacros; m++)
    {
      Macro_t *Macro = MacroList->Data[m];
      bool IsMatch = Token->IsMatch(Macro->Token);
      if (Token->IsIdent() && IsMatch)
      {
        FoundMacro = true;

        // NOTE(aen): Don't add any more tokens to the output list until we
        // parse the full macro usage and know the substitution values.

        // Log("Found macro for expansion: ");
        // Token->Debug();
        In->NextToken();

        TokenList_t *SubList = new TokenList_t[Macro->ParamList.NumTokens];
        In->ExpectToken('(', '[');
        for (u64 p = 0; p < Macro->ParamList.NumTokens; p++)
        {
          TokenList_t *SL = SubList + p;
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
          Token_t *TokenToWrite = Macro->Expansion.AtToken();

          s64 Match = -1;
          for (u64 P = 0; P < Macro->ParamList.NumTokens; P++)
          {
            Token_t *T = Macro->ParamList.Get(P);
            if (TokenToWrite->IsMatch(T))
            {
              Match = P;
              break;
            }
          }

          if (Match >= 0)
          {
            TokenList_t *SLP = SubList + Match;
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
