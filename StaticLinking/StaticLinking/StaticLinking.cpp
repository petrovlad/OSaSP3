#include <cstdio>
#include <tchar.h>
#include <Windows.h>

#define TARGET_EXE "AppThatOutputString.exe"
#define TARGET_STR "I hate programming!"
#define NEW_STR "I love programming!"

extern "C" __declspec(dllexport) int __stdcall ReplaceString(LPTSTR szTargetProcessName, const char* stringToFind, const char* stringToReplace);

int main()
{
    TCHAR processName[] = TEXT(TARGET_EXE);
    int pid;
    if ((pid = ReplaceString(processName, TARGET_STR, NEW_STR)) != -1) {
        printf("String replaced successfully. Target process ID = %d.\n", pid);
    }
    else {
        printf("Failed to replace string.\n");
    }
}