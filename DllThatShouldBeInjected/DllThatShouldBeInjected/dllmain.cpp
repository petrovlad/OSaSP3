// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include <tlhelp32.h>
#include <tchar.h>
#include <cstdlib>
#include <psapi.h>

#include <cstdio>

#define _CRT_SECURE_NO_WARNINGS
#define TARGET_EXE "AppThatOutputString.exe"
#define TARGET_STR "I hate programming!"
#define NEW_STR "I love programming!"

extern "C" __declspec(dllexport) int __stdcall ReplaceString(LPTSTR szTargetProcessName, const char* stringToFind, const char* stringToReplace);
extern "C" __declspec(dllexport) int __stdcall MyStrStr(const char* str, const char* substr, int l1, int l2);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        TCHAR processName[] = TEXT(TARGET_EXE);
        int result = ReplaceString(processName, TARGET_STR, NEW_STR);
        if (result != -1) {
            printf("Success\n");
        }
        else {
            printf("Fuck this\n");
        }
    }
    return TRUE;
}

// returns position of substr in str or -1
extern "C" __declspec(dllexport) int __stdcall MyStrStr(const char* str, const char* substr, int l1, int l2) {
    int i1 = 0, i2 = 0;
    while (i1 < l1) {
        if (str[i1] == substr[0]) {
            int bufi1 = i1;
            i2 = 0;
            while ((str[i1] == substr[i2]) && (i2 < l2)) {
                i1++;
                i2++;
            }
            if (i2 == l2) {
                return bufi1;
            }
            i1 = bufi1;
        }
        i1++;
    }
    return -1;
}

// returns pid of target process if ok, otherwise returns -1
extern "C" __declspec(dllexport) int __stdcall ReplaceString(LPTSTR szTargetProcessName, const char* stringToFind, const char* stringToReplace) {
    // create and initialize PROCESSENTRY structure
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    // creating handle for processes snapshot
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return -1;
    }
    // try to get processes into structure
    if (Process32First(hProcessSnap, &entry) != TRUE) {
        CloseHandle(hProcessSnap);
        return -1;
    }
    //
    DWORD pid = -1;
    // cycle for each process; try to find target process
    while (Process32Next(hProcessSnap, &entry) == TRUE) {
        // first we open process, then check name. if names are equals, find and replace string. after that close handle
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, entry.th32ProcessID);
        if (hProcess != NULL) {
            // variable that stores current process name
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
            // vars that are needed in getting process'es data
            HMODULE hMod;
            DWORD cbNeeded;
            // if we can enumerate process - try to get its name
            if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
            }
            // comparing processes names
            if (_tcscmp(szProcessName, szTargetProcessName) == 0) {
                // store this. it can be useful
                pid = entry.th32ProcessID;

                unsigned char* p = nullptr;
                MEMORY_BASIC_INFORMATION info;
                for (p = nullptr; (VirtualQueryEx(hProcess, p, &info, sizeof(info)) == sizeof(info)); p += info.RegionSize)
                {
                    char* buffer = nullptr;
                    if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE))
                    {
                        SIZE_T bytes_read;
                        buffer = (char*)malloc(info.RegionSize);
                        if (buffer == NULL) {
                            return -1;
                        }
                        memset(buffer, 0, info.RegionSize);
                        ReadProcessMemory(hProcess, p, &buffer[0], info.RegionSize, &bytes_read);
                        int pos = MyStrStr(buffer, stringToFind, bytes_read, 20);
                        if (pos != -1) {
                            //_tprintf(TEXT("YEAH BEACH"));
                            char* addr = static_cast<char*>(info.BaseAddress) + pos;
                            const char* newStr = stringToReplace;
                            if (WriteProcessMemory(hProcess, (LPVOID)addr, newStr, 20, NULL) == 0) {
                                return -1;
                            }
                            else {
                                return pid;
                            }
                        }
                    }
                }
                CloseHandle(hProcess);
                break;
            }
            CloseHandle(hProcess);
        }
    }
    CloseHandle(hProcessSnap);
    return pid;
}
