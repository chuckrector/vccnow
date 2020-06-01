#include "string.hpp"
#include "mem.hpp"

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
      if (MemMatches(C, This, ThisLength))
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
StringsMatch(char *A, char *B, u64 Limit)
{
  b64 Result;
  u64 Counter = 0;

  while (*A && *A == *B)
  {
    if (Limit && ++Counter >= Limit)
    {
      break;
    }

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