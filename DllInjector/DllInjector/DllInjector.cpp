#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <cstdio>
#include <psapi.h>

#define TARGET_PROCESS_NAME "AppToInject.exe"
#define DLL_NAME "C:\\Users\\vladi\\source\\repos\\DllThatShouldBeInjected\\Debug\\DllThatShouldBeInjected.dll"

int FindPID(LPTSTR targetProcessName) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hProcessSnapshot == INVALID_HANDLE_VALUE) {
		printf("Can't get snapshot. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	if (Process32First(hProcessSnapshot, &entry) == FALSE) {
		printf("Can't execute Process32First. GetLastError code: %d.\n", GetLastError());
		CloseHandle(hProcessSnapshot);
		return -1;
	}

	int pid = -1;
	while (Process32Next(hProcessSnapshot, &entry) == TRUE) {
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
		if (hProcess != NULL) {
			TCHAR processName[MAX_PATH] = TEXT("<unknown>");
			HMODULE hMod;
			DWORD needed;

			if (EnumProcessModules(hProcess, &hMod, sizeof(HMODULE), &needed)) {
				GetModuleBaseName(hProcess, hMod, processName, MAX_PATH);
			}

			CloseHandle(hProcess);
			if (_tcscmp(processName, targetProcessName) == 0) {
				pid = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(hProcessSnapshot);
	return pid;
}


int main()
{
	// 1. find process
	// 2. open the process
	// 3. get LoadLibraryA from kernel32
	// 4. allocate memory in target process for dllName
	// 5. write
	// 6. create remote thread
	TCHAR targetProcessName[] = TEXT(TARGET_PROCESS_NAME);
	int pid = FindPID(targetProcessName);
	if (pid == -1) {
		printf("Failed to find pid.\n");
		return -1;
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL) {
		printf("OpenProcess failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	// "Вытягивание" функции из системной библиотеки для динамической  
	// подгрузки DLL в адресное пространство открытого процесса
	HMODULE kernel32Module = GetModuleHandle(TEXT("kernel32.dll"));
	if (kernel32Module == NULL) {
		printf("GetModuleHandle failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	LPVOID loadLibraryFunction = (LPVOID)GetProcAddress(kernel32Module, "LoadLibraryA");
	if (loadLibraryFunction == NULL) {
		printf("GetProcAddress failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	// Выделение участка памяти размером strlen(_dll_name) для последующей 
	// записи имени библеотеки в память процесса.
	LPVOID alloc = VirtualAllocEx(hProcess, 0, strlen(DLL_NAME), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (alloc == NULL) {
		printf("VirtualAllocEx failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	// Запись имени инжектируемой DLL в память
	BOOL w = WriteProcessMemory(hProcess, (LPVOID)alloc, DLL_NAME, strlen(DLL_NAME), 0);
	if (w == NULL) {
		printf("WriteProcessMemory failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	// Создание "удаленного" потока в адресном пространстве
	// открытого процесса и последующая подгрузка нашей DLL.
	HANDLE thread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)loadLibraryFunction, (LPVOID)alloc, 0, 0);
	if (thread == NULL) {
		printf("CreateRemoteThread failed. GetLastError code: %d.\n", GetLastError());
		return -1;
	}

	CloseHandle(hProcess);
	return 0;
}
