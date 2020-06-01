#ifndef VCC_HPP
#define VCC_HPP

#include "types.hpp"

void PreStartupFiles(char *FilenameWithoutExtension);
void PostStartupFiles();
void WriteOutput(const char *FilenameWithoutExtension);
void WriteEffectOutput();
void WriteMagicOutput();
void WriteScriptOutput();
void WriteScriptOffsetTableToBuffer(buffer *Output);

#endif // VCC_HPP