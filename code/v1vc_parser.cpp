#include "v1vc_parser.hpp"
#include "compile.hpp"
#include "v1vc_token.hpp"

parser::parser() {}
parser::parser(char *S) { Reset(NewBuffer(S)); }
parser::parser(buffer *B) { Reset(B); }

// NOTE(aen): Tokenizes from the current C, wherever that lies
void
parser::ToTokenList(token_list *TokenList)
{
  // NOTE(aen): If T=Token and W=Whitespace, all code is a series of WTW.
  // W flavors coalesce, so WWWTWWTW == WTWTW. You can loop WT or TW to avoid
  // redundant W work, with one W before or after.
  SkipWhite();
  while (!AtEnd())
  {
    token *Token = NewToken();
    Token->Start = CurrentOffset();
    Token->Text = (char *)Data + Token->Start;
    Token->Line = Line;
    Token->Column = Column;

    if (IsLetter())
    {
      while (!AtEnd() && IsIdent())
        Next();
      Token->Type = TT_IDENT;
    }
    else if (IsDigit())
    {
      while (!AtEnd() && IsDigit())
        Next();
      Token->Type = TT_NUMBER;
    }
    else if (IsSpecial())
    {
      if (*C == '"')
      {
        Next();
        while (!AtEnd() && *C != '"')
          Next();

        if (!AtEnd() && *C == '"')
          Next();
        else
          Fail(
              "L%d:C%d: This text is missing a closing quote: %.*s[...]\n",
              Token->Line,
              Token->Column,
              Token->Length,
              Token->Text);

        Token->Type = TT_STRING;
      }
      else if (
          AtText("+=", 2) || AtText("-=", 2) || AtText("*=", 2) ||
          AtText("/=", 2) || AtText("%=", 2) || AtText("<=", 2) ||
          AtText(">=", 2) || AtText("==", 2) || AtText("++", 2) ||
          AtText("--", 2) || AtText("&&", 2) || AtText("||", 2))
      {
        Next(2);
        Token->Type = TT_SYMBOL;
      }
      else if (
          *C == '{' || *C == '}' || *C == '(' || *C == ')' || *C == ',' ||
          *C == ';' || *C == '#' || *C == '@' || *C == '[' || *C == ']' ||
          *C == '+' || *C == '-' || *C == '=' || *C == '<' || *C == '>' ||
          *C == ':' || *C == '!' || *C == '*' || *C == '/' || *C == '%' ||
          *C == '&' || *C == '|')
      {
        Next();
        Token->Type = TT_SYMBOL;
      }
      else
        Fail(
            "L%d:C%d: Unrecognized text: %.*s[...] '%c' (%d)\n",
            Token->Line,
            Token->Column,
            Token->Length,
            Token->Text,
            Token->Text[0],
            Token->Text[0]);
    }
    else
      Fail(
          "L%d:C%d: Unrecognized char type: %d - %.*s[...] '%c' (%d)\n",
          CharTypeLookup[(u64)*C],
          Token->Line,
          Token->Column,
          Token->Length,
          Token->Text,
          Token->Text[0],
          Token->Text[0]);

    Token->End = CurrentOffset();
    Token->Length = Token->End - Token->Start;
    TokenList->AddToken(Token);

    SkipWhite();
  }
}

void
parser::CalcPath(char *Filename)
{
  // Log("CalcPath: %s\n", Filename);
  if (!Path)
    Path = (char *)NewTempBuffer(TEMP_BUFFER_SIZE);
  SetPath(Filename, Path, TEMP_BUFFER_SIZE);
  // Log("CalcPath: Path %s\n", Path);
}

void
parser::Load(char *Filename)
{
  CalcPath(Filename);

  // NOTE(aen): Filename could be a relative path. Strip out the filename
  char FilenameOnly[_MAX_PATH];
  char ExtOnly[_MAX_EXT];
  _splitpath_s(
      Filename, NULL, 0, NULL, 0, FilenameOnly, _MAX_PATH, ExtOnly, _MAX_PATH);

  sprintf_s(
      TempBuffer, TEMP_BUFFER_SIZE, "%s%s%s", Path, FilenameOnly, ExtOnly);
  buffer *Buffer = ::LoadEntireFile(TempBuffer);
  Reset(Buffer);
}

void
parser::Reset(char *S)
{
  Reset(NewBuffer(S));
}
void
parser::Reset(buffer *B)
{
  this->Data = B->Data;
  this->Length = B->Length;
  C = Data;
  Line = 1;
  Column = 1;
}

u64
parser::CurrentOffset()
{
  return C - Data;
}
u8 *
parser::End()
{
  return Data + this->Length;
};
b64
parser::AtEnd()
{
  return C >= Data + Length;
}
void
parser::Debug()
{
  Log("Parser: @%ld c='%c'\n", CurrentOffset(), *C);
}

void
parser::Next(s64 Delta)
{
  s64 Direction = 1;
  if (Delta < 0)
    Direction = -1;
  u8 *Target = C + Delta;
  while (C != Target)
  {
    if (*C == '\t')
      Column += TabSize;
    else if (*C == '\n')
    {
      Line++;
      Column = 1;
    }
    else
      Column++;
    this->C += Direction;
  }
}

b64
parser::IsWhite()
{
  return *C <= ' ';
}
b64
parser::IsLetter()
{
  return IsCharType(*C, LETTER);
}
b64
parser::IsDigit()
{
  return IsCharType(*C, DIGIT);
}
b64
parser::IsSpecial()
{
  return IsCharType(*C, SPECIAL);
}
b64
parser::IsIdent()
{
  return IsLetter() || IsDigit();
}

b64
parser::AtText(char Char)
{
  return !AtEnd() && *C == Char;
}
b64
parser::AtText(char *Text, u64 L)
{
  return !AtEnd() && C + L <= End() && MemMatches(C, (u8 *)Text, L);
}

void
parser::SkipPast(char *Text, u64 L)
{
  u8 *Head = C;

  while (!AtEnd() && !AtText(Text, L))
    Next();

  if (AtEnd())
    return;

  if (!AtText(Text, L))
    Fail(
        "Expected '%s' but never found it @%ld: %.*s[...]\n",
        Text,
        C - Data,
        20,
        Head);

  Next(L);
}

void
parser::SkipWhite()
{
  if (!IsWhite() && !AtText("//", 2) && !AtText("/*", 2))
    return;

  while (!AtEnd())
  {
    if (IsWhite())
      Next();
    else if (AtText("//", 2))
    {
      Next(2);
      SkipPast("\n", 1);
    }
    else if (AtText("/*", 2))
    {
      Next(2);
      SkipPast("*/", 2);
    }
    else
      break;
  }
}

void
parser::Expect(char Char)
{
  if (AtText(Char))
    Next();
  else
    Fail(
        "Expected '%c' (%d) but got '%c' (%d) @%ld\n",
        Char,
        Char,
        *C,
        *C,
        CurrentOffset());
}

void
parser::Expect(char *Text, u64 L)
{
  if (AtText(Text, L))
    Next(L);
  else
    Fail(
        "Expected '%s' but got '%.*s[...] @%ld c='%c' (%d)\n",
        Text,
        20,
        C,
        CurrentOffset(),
        *C,
        *C);
}
