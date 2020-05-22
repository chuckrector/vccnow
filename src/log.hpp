#ifndef LOG_HPP
#define LOG_HPP

void Fail(const char *Format, ...);
void Log(const char *Format, ...);

enum DebugLevel_e
{
  NONE,
  LOW,
  MEDIUM,
  HIGH
};

DebugLevel_e DebugLevel = NONE;
void DebugLog(DebugLevel_e Level, const char *Format, ...);

#endif // LOG_HPP