#ifndef V1VC_PARSER_HPP
#define V1VC_PARSER_HPP

#include "v1vc_token.hpp"

struct parser
{
  char *Path = 0; // NOTE(aen): If data from disk
  u8 *Data = 0;
  u8 *C = 0;
  u64 Length = 0;

  u64 Line = 0;
  u64 Column = 0;
  u64 TabSize = 8;

  void ToTokenList(token_list *TokenList);

  parser();
  parser(char *S);
  parser(buffer *B);

  void Reset(char *S);
  void Reset(buffer *B);
  void CalcPath(char *Filename);
  void Load(char *Filename);

  b64 IsWhite();
  b64 IsLetter();
  b64 IsDigit();
  b64 IsSpecial();
  b64 IsIdent();

  u8 *End();
  b64 AtEnd();
  b64 AtText(char Char);
  b64 AtText(char *Text, u64 Length);
  void Expect(char Char);
  void Expect(char *Text, u64 Length);
  void SkipPast(char *Text, u64 Length);
  void SkipWhite();

  void Next(s64 Delta = 1); // NOTE(aen): Important for line/col tracking

  u64 CurrentOffset();
  void Debug();
};

#endif // V1VC_PARSER_HPP
