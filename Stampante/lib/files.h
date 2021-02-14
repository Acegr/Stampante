#include <windows.h>
bool CreateLogFile(HANDLE*);
bool WriteToLog(const char*, HANDLE);
bool TimeString(char*);
bool Log(char*, HANDLE);
