#include <cstdio>
#include <Windows.h>
#include <tchar.h>

#define TARGET_EXE "AppThatOutputString.exe"
#define TARGET_STR "I hate programming!"
#define NEW_STR "I love programming!"

typedef int (__stdcall *REPLACESTRINGFUNCTION)(LPTSTR, const char*, const char*);

int main()
{
	HINSTANCE hModule = LoadLibrary(L"..\\Dependencies\\dll\\DllThatReplacesString.dll");
	if (hModule == NULL) {
		printf("Failed to load library. GetLastError = %d\n", GetLastError());
		return -1;
	}
	REPLACESTRINGFUNCTION replaceStringFunction = (REPLACESTRINGFUNCTION)GetProcAddress(hModule, "_ReplaceString@12");
	if (replaceStringFunction == NULL) {
		printf("Failed to get function address. GetLastError = %d\n", GetLastError());
		FreeLibrary(hModule);
		return -1;
	}
	TCHAR processName[] = TEXT(TARGET_EXE);
	int pid = replaceStringFunction(processName, TARGET_STR, NEW_STR);
	if (pid != -1) {
		printf("String replaced successfully. Target process ID = %d.\n", pid);
	}
	else {
		printf("Failed to replace string.\n");
	}
	FreeLibrary(hModule);
}