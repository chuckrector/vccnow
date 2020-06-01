#ifndef STRING_HPP
#define STRING_HPP

#include "types.hpp"

u64 StringLength(char *String);
char *StringFindLast(char *Str, char *This);
b64 StringEndsWith(char *String, char *Suffix);
char *StringCopy(char *Source, char *Dest);
b64 StringsMatch(char *A, char *B, u64 Limit = 0);
void StringToUpperCase(char *String);

#endif // STRING_HPP