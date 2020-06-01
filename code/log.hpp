#ifndef LOG_HPP
#define LOG_HPP

void Fail(char *Format, ...);
void Log(char *Format, ...);

enum DebugLevel_e
{
  NONE,
  LOW,
  MEDIUM,
  HIGH
};

DebugLevel_e DebugLevel = NONE;
void DebugLog(DebugLevel_e Level, char *Format, ...);

#endif // LOG_HPP