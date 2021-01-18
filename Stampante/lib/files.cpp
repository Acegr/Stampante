#include <string.h>
#include "files.h"



bool CreateLogFile(HANDLE *LogFileHandle)
{
    SYSTEMTIME Time; //The log's name has format logSSMMHHDDMMYY.txt, so we need
                     //the system time
    LPSYSTEMTIME TimePointer = &Time;
    GetSystemTime(TimePointer);
    char LogName [20] {};
    LogName[0]  = 'l';
    LogName[1]  = 'o';
    LogName[2]  = 'g';
    LogName[3]  = (TimePointer->wDay    / 10)          + 48;
    LogName[4]  = (TimePointer->wDay    % 10)          + 48;
    LogName[5]  = (TimePointer->wMonth  / 10)          + 48;
    LogName[6]  = (TimePointer->wMonth  % 10)          + 48;
    LogName[7]  = ((TimePointer->wYear  % 100) / 10)   + 48;
    LogName[8]  = (TimePointer->wYear   % 10)          + 48;
    LogName[9]  = (TimePointer->wHour   / 10)          + 48;
    LogName[10] = (TimePointer->wHour   % 10)          + 48;
    LogName[11] = (TimePointer->wMinute / 10)          + 48;
    LogName[12] = (TimePointer->wMinute % 10)          + 48;
    LogName[13] = (TimePointer->wSecond / 10)          + 48;
    LogName[14] = (TimePointer->wSecond % 10)          + 48;
    LogName[15] = '.';
    LogName[16] = 't';
    LogName[17] = 'x';
    LogName[18] = 't';
    LogName[19] =  0;
    *LogFileHandle = CreateFile(LogName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if(*LogFileHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool WriteToLog(const char* Message, HANDLE LogFileHandle) //Writes the message to the log file
{
    DWORD MessageNumberOfBytesWritten = 0;
    WriteFile(LogFileHandle,
              Message,
              strlen(Message),
              &MessageNumberOfBytesWritten,
              NULL);
    if(MessageNumberOfBytesWritten != strlen(Message))
    {
        return false;
    }
    return true;
}

bool TimeString(char *Out) /*Outputs the system time as a string.
                            *Argument needs to be 9 bytes*/
{
    SYSTEMTIME Time;
    LPSYSTEMTIME TimePointer = &Time;
    GetSystemTime(TimePointer);
    Out[0] = (TimePointer->wHour   / 10) + 48;
    Out[1] = (TimePointer->wHour   % 10) + 48;
    Out[2] = ':';
    Out[3] = (TimePointer->wMinute / 10) + 48;
    Out[4] = (TimePointer->wMinute % 10) + 48;
    Out[5] = ':';
    Out[6] = (TimePointer->wSecond / 10) + 48;
    Out[7] = (TimePointer->wSecond % 10) + 48;
    Out[8] = 0;
    return true;
}

bool Log(char *Message, HANDLE LogFileHandle)
//writes to the log file the time in the HH:MM:SS format followed by a colon,
//then the message, then a newline
{
    char Time[9];
    TimeString(Time);
    DWORD TimeNumberOfBytesWritten = 0;
    WriteFile(LogFileHandle,
              Time,
              8,
              &TimeNumberOfBytesWritten,
              NULL);
    if(TimeNumberOfBytesWritten != 8)
    {
        return false;
    }
    char colon [2] = {':', ' '};
    DWORD ColonNumberOfBytesWritten = 0;
    WriteFile(LogFileHandle,
              &colon,
              2,
              &ColonNumberOfBytesWritten,
              NULL);
    if(ColonNumberOfBytesWritten != 2)
    {
        return false;
    }
    DWORD MessageNumberOfBytesWritten = 0;
    WriteFile(LogFileHandle,
              Message,
              strlen(Message),
              &MessageNumberOfBytesWritten,
              NULL);
    if(MessageNumberOfBytesWritten != strlen(Message))
    {
        return false;
    }
    char *newLine = "\r\n";
    DWORD newLineNumberOfBytesWritten = 0;
    WriteFile(LogFileHandle,
              newLine,
              2,
              &newLineNumberOfBytesWritten,
              NULL);
    if(newLineNumberOfBytesWritten != 2)
    {
        return false;
    }
    return true;
}
