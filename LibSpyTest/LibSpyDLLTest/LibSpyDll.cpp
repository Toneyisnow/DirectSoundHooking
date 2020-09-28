/***************************************************************
Module name: LibSpyDll.cpp
Copyright (c) 2003 Robert Kuster

Notice:	If this code works, it was written by Robert Kuster.
		Else, I don't know who wrote it.

		Use it on your own risk. No responsibilities for
		possible damages of even functionality can be taken.
***************************************************************/
#include "pch.h"

#include <iostream>
// #include <dsound.h>

#include <windows.h>
#include "LibSpyDll.h"

static VOID(WINAPI* TrueSleep)(DWORD dwMilliseconds) = Sleep;
// Detour function that replaces the Sleep API. 
VOID WINAPI TimedSleep(DWORD dwMilliseconds)
{
    TrueSleep(dwMilliseconds);
}


//-------------------------------------------------------
// shared data 
// Notice:	seen by both: the instance of "LibSpy.dll" mapped
//			into the remote process as well as by the instance
//			mapped into our "LibSpy.exe"
#pragma data_seg (".shared")
HWND	g_hPwdEdit = 0;
char	g_szPassword[128] = { '\0' };
HHOOK hhk;
#pragma data_seg ()

#pragma comment(linker,"/SECTION:.shared,RWS") 



HINSTANCE hinst;

//#pragma comment(linker, "/SECTION:.shared,RWS") compiler error in VC++ 2008 express

LRESULT CALLBACK wireKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code < 0) {
        return CallNextHookEx(0, code, wParam, lParam);
    }
    Beep(1000, 20);
    return CallNextHookEx(hhk, code, wParam, lParam);
}

extern "C" __declspec(dllexport) void install(unsigned long threadID) {
    hhk = SetWindowsHookEx(WH_KEYBOARD, wireKeyboardProc, hinst, threadID);
}
extern "C" __declspec(dllexport) void uninstall() {
    UnhookWindowsHookEx(hhk);
}

BOOL WINAPI DllMain(__in HINSTANCE hinstDLL, __in  DWORD dwReason, __in  LPVOID lpvReserved) {
    hinst = hinstDLL;

    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueSleep, TimedSleep);
        DetourTransactionCommit();
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueSleep, TimedSleep);
        DetourTransactionCommit();
    }
    return TRUE;
}


/// <summary>
/// Testing hooking Keyboard
/// </summary>
HHOOK _hook;

KBDLLHOOKSTRUCT kbdStruct;

LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN)
        {
            kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);

            if (kbdStruct.vkCode == VK_END)
            {
                MessageBox(NULL, (LPCWSTR)L"END is pressed!", (LPCWSTR)L"Key Pressed", MB_ICONINFORMATION);
            }
        }
    }

    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        MessageBox(NULL, (LPCWSTR)L"Failed to install hook!", (LPCWSTR)L"Error", MB_ICONERROR);
    }
}

void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}

//-------------------------------------------------------
// DllMain
// Notice: retrieves the password, when mapped into the 
//		   remote process (g_hPwdEdit != 0);
//
BOOL APIENTRY DllMain2(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if ((ul_reason_for_call == DLL_PROCESS_ATTACH) && g_hPwdEdit) {
		::MessageBeep(MB_OK);
		::SendMessage(g_hPwdEdit, WM_GETTEXT, 128, (LPARAM)g_szPassword);
	}

    SetHook();

    /*
    std::cout << "Hello World!\n";

    LPDIRECTSOUND8 sound = NULL;
    HRESULT hr = DirectSoundCreate8(NULL, &sound, NULL);
    if (FAILED(hr))
    {
        //UE_LOG(LogVoiceCapture, Warning, TEXT("Failed to init DirectSound %d"), hr);
        return false;
    }
    */
    // bool bHmdAvailable = IModularFeatures::Get().IsModularFeatureAvailable(IHeadMountedDisplayModule::GetModularFeatureName());

    
	return TRUE;
}

