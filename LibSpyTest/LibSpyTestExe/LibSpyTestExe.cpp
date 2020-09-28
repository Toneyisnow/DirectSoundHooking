// LibSpyTestExe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <windows.h>
#include <stdio.h>     // sprintf

#include <cstdio>
#include <tlhelp32.h>
#include <chrono>
#include <thread>

#include <comdef.h>  // you will need this

#include "../LibSpyDLLTest/LibSpyDll.h"

//-----------------------------------------------
// global variables & forward declarations
//
HINSTANCE	hInst;

HWND		hStatic;
HWND		hWndOld;

bool		bOverPasswdEdit = false;	// cursor over edit control with 
										// ES_PASSWORD style set?


HANDLE GetHandleByProcessName(const char * targetProcess)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			const WCHAR* filename = entry.szExeFile;
			char buffer[260];
			int ret;
			ret = wcstombs(buffer, filename, sizeof(buffer));

			if (_stricmp(buffer, targetProcess) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

				return hProcess;
			}
		}
	}
}

unsigned long GetTargetThreadIdFromWindow(LPCWSTR className, LPCWSTR windowName)
{
	HWND targetWnd;
	HANDLE hProcess;
	unsigned long processID = 0;

	targetWnd = FindWindow(className, windowName);
	return GetWindowThreadProcessId(targetWnd, &processID);
}

//-----------------------------------------------
// InjectDll
// Notice: Loads "LibSpy.dll" into the remote process
//		   (via CreateRemoteThread & LoadLibrary)
//
//		Return value:	1 - success;
//						0 - failure;
//
int InjectDll(HANDLE hProcess)
{
	HANDLE hThread;
	/// char szLibPath[_MAX_PATH];

	void* pLibRemote = 0;	// the address (in the remote process) where
							// szLibPath will be copied to;
	DWORD  hLibModule = 0;	// base adress of loaded module (==HMODULE);

	HMODULE hKernel32 = ::GetModuleHandle(L"Kernel32");


	// Get full path of "LibSpy.dll"
	/*
	LPWSTR szLibPath = NULL;
	if (!GetModuleFileName(hInst, szLibPath, _MAX_PATH))
		return false;
	char buffer[260];
	int ret;
	ret = wcstombs(buffer, szLibPath, sizeof(buffer));

	strcpy(strstr(buffer, ".exe"), ".dll");
	*/

	LPWSTR szLibPath = (LPWSTR)L"C:\\Users\\charl\\source\\repos\\LibSpyTest\\Debug\\LibSpyDllTest.dll";

	// 1. Allocate memory in the remote process for szLibPath
	// 2. Write szLibPath to the allocated memory
	pLibRemote = ::VirtualAllocEx(hProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE);
	if (pLibRemote == NULL)
		return false;
	::WriteProcessMemory(hProcess, pLibRemote, (void*)szLibPath, sizeof(szLibPath), NULL);


	// Load "LibSpy.dll" into the remote process 
	// (via CreateRemoteThread & LoadLibrary)
	hThread = ::CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32, "LoadLibraryA"),
		pLibRemote, 0, NULL);
	if (hThread == NULL)
		goto JUMP;

	::WaitForSingleObject(hThread, INFINITE);

	// Get handle of loaded module
	::GetExitCodeThread(hThread, &hLibModule);
	::CloseHandle(hThread);

JUMP:
	::VirtualFreeEx(hProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE);
	if (hLibModule == NULL)
		return false;


	// Unload "LibSpy.dll" from the remote process 
	// (via CreateRemoteThread & FreeLibrary)
	hThread = ::CreateRemoteThread(hProcess,
		NULL, 0,
		(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32, "FreeLibrary"),
		(void*)hLibModule,
		0, NULL);
	if (hThread == NULL)	// failed to unload
		return false;

	::WaitForSingleObject(hThread, INFINITE);
	::GetExitCodeThread(hThread, &hLibModule);
	::CloseHandle(hThread);

	// return value of remote FreeLibrary (=nonzero on success)
	return hLibModule;
}

void injectDllToProcess()
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			const WCHAR* filename = entry.szExeFile;
			char buffer[260];
			int ret;
			ret = wcstombs(buffer, filename, sizeof(buffer));

			if (_stricmp(buffer, "KuGou.exe") == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

				// Do stuff..
				InjectDll(hProcess);

				std::this_thread::sleep_for(std::chrono::seconds(20));

				CloseHandle(hProcess);
			}
		}
	}

	CloseHandle(snapshot);


	std::cout << "Completed!\n";
}


int main(int argc, char* argv[]) {
	unsigned long threadID = GetTargetThreadIdFromWindow((LPCWSTR)L"Notepad", (LPCWSTR)L"Untitled - Notepad");
	printf("TID: %i", threadID);

	HANDLE h1 = GetHandleByProcessName("notepad.exe");
	threadID = GetThreadId(h1);

	InjectDll(h1);

	HINSTANCE hinst = LoadLibrary(L"LibSpyDllTest.dll");

	if (hinst) {
		typedef void (*Install)(unsigned long);
		typedef void (*Uninstall)();

		Install install = (Install)GetProcAddress(hinst, "install");
		Uninstall uninstall = (Uninstall)GetProcAddress(hinst, "uninstall");

		install(threadID);

		//// Sleep(20000);

		uninstall();
	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
