#include "string.hpp"

u64
StringLength(char *String)
{
  u64 Result = 0;

  char *C = String;
  while (*C)
  {
    ++C;
  }

  Result = C - String;
  return (Result);
}

char *
StringFindLast(char *Str, char *This)
{
  char *Result = 0;

  u64 StrLength = StringLength(Str);
  u64 ThisLength = StringLength(This);

  if (ThisLength < StrLength)
  {
    char *C = Str + StrLength - ThisLength;
    while (C != Str)
    {
      if (!memcmp(C, This, ThisLength))
      {
        Result = C;
        break;
      }

      --C;
    }
  }

  return (Result);
}

b64
StringEndsWith(char *String, char *Suffix)
{
  b64 Result = false;

  char *Found = StringFindLast(String, Suffix);
  Result = (Found != 0);

  return (Result);
}

char *
StringCopy(char *Source, char *Dest)
{
  char *Result = Dest;
  while (*Dest++ = *Source++) {}
  return (Result);
}

b64
StringsMatch(char *A, char *B)
{
  b64 Result;

  while (*A && *A == *B)
  {
    ++A;
    ++B;
  }

  Result = (*A == *B);
  return (Result);
}

void
StringToUpperCase(char *String)
{
  char *C = String;

  while (*C)
  {
    if (*C >= 'a' && *C <= 'z')
    {
      *C -= ('a' - 'A');
    }

    ++C;
  }
}