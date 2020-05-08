#ifndef UTIL_HPP
#define UTIL_HPP

#include "types.hpp"
#include <stdio.h>

#define TEMP_BUFFER_SIZE (1024 * 2)
char TempBuffer[TEMP_BUFFER_SIZE];

// void strupr(char *s);
bool64 Exist(const char *Filename);
u64 FileSize(FILE *File);
Buffer_t *Load(const char *Filename);
void SetPath(const char *Filename, char *Path, u64 PathLength);

#endif // UTIL_HPP