#ifndef V1VC_PARSER_HPP
#define V1VC_PARSER_HPP

#include "v1vc_token.hpp"

struct Parser_t
{
  char *Path = 0; // NOTE(aen): If data from disk
  u8 *Data = 0;
  u8 *C = 0;
  u64 Length = 0;

  u64 Line = 0;
  u64 Column = 0;
  u64 TabSize = 8;

  void ToTokenList(TokenList_t *TokenList);

  Parser_t();
  Parser_t(const char *S);
  Parser_t(Buffer_t *B);

  void Reset(const char *S);
  void Reset(Buffer_t *B);
  void CalcPath(const char *Filename);
  void Load(const char *Filename);

  bool64 IsWhite();
  bool64 IsLetter();
  bool64 IsDigit();
  bool64 IsSpecial();
  bool64 IsIdent();

  u8 *End();
  bool64 AtEnd();
  bool64 AtText(char Char);
  bool64 AtText(const char *Text, u64 Length);
  void Expect(char Char);
  void Expect(const char *Text, u64 Length);
  void SkipPast(const char *Text, u64 Length);
  void SkipWhite();

  void Next(s64 Delta = 1); // NOTE(aen): Important for line/col tracking

  u64 CurrentOffset();
  void Debug();
};

#endif // V1VC_PARSER_HPP
