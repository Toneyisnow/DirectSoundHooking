// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <stdio.h>
#include <windows.h>

#include <mmsystem.h>
#include <dsound.h>

HHOOK _hook;

KBDLLHOOKSTRUCT kbdStruct;

static VOID(WINAPI* TrueSleep)(DWORD dwMilliseconds) = Sleep;

EXTERN_C_START

static _Check_return_ HRESULT (WINAPI* TrueDirectSoundCreate)(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter) = DirectSoundCreate;
//// __stdcall GetCurrentPosition(_Out_opt_ LPDWORD pdwCurrentPlayCursor, _Out_opt_ LPDWORD pdwCurrentWriteCursor);

typedef HRESULT(WINAPI* DirectSoundCreateDef)(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter);
typedef HRESULT (WINAPI* GetCurrentPositionDef)(LPDWORD lpdwCurrentPlayCursor, LPDWORD lpdwCurrentWriteCursor);


EXTERN_C_END

// Detour function that replaces the Sleep API. 
VOID WINAPI TimedSleep(DWORD dwMilliseconds)
{
    

    TrueSleep(dwMilliseconds);
}

HRESULT WINAPI hkDirectSoundCreate(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter)
{
    FILE* file;
    fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");
    fprintf(file, "Called hkDirectSoundCreate.\n");
    fclose(file);

    return DirectSoundCreate(pcGuidDevice, ppDS, pUnkOuter);
}

HRESULT hkGetCurrentPosition(LPDWORD lpdwCurrentPlayCursor, LPDWORD lpdwCurrentWriteCursor)
{
    FILE* file;
    fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");
    fprintf(file, "Called hkGetCurrentPosition.\n");
    fclose(file);

    ///IDirectSoundBuffer buffer = NULL;
    ///return buffer.GetCurrentPosition(lpdwCurrentPlayCursor, lpdwCurrentWriteCursor);
    return 0;
}

LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    FILE* file;
    fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");
    fprintf(file, "Key pressed.\n");
    fclose(file);

    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN)
        {
            kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);

            FILE* file;
            fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");
            fprintf(file, "Key pressed.\n");
            fclose(file);

            if (kbdStruct.vkCode == VK_END)
            {
                MessageBox(NULL, (LPCWSTR)L"END is pressed!", (LPCWSTR)L"Key Pressed", MB_ICONINFORMATION);
            }
        }
    }

    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

LRESULT CALLBACK WireKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code < 0) {
        return CallNextHookEx(0, code, wParam, lParam);
    }

    FILE* file;
    fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");
    fprintf(file, "Clicked button.\n");
    fclose(file);

    return CallNextHookEx(_hook, code, wParam, lParam);
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



bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len)
{
    if (len < 5) return false;

    DWORD curProtection;

    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

    uintptr_t relativeAddress = dst - src - 5;

    *src = 0xE9;

    *(uintptr_t*)(src + 1) = relativeAddress;

    VirtualProtect(src, len, curProtection, &curProtection);

    return true;
}

BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len)
{
    if (len < 5) return 0;

    BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    memcpy_s(gateway, len, src, len);

    uintptr_t gatewayRelativeAddress = src - gateway - 5;

    *(gateway + len) = 0xE9;

    *(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddress;

    Detour32(src, dst, len);

    return gateway;
}

void SetHook(FILE* traceFile)
{
    fprintf(traceFile, "Start SetHook.\n");

    DirectSoundCreateDef oDirectSoundCreate = (DirectSoundCreateDef)GetProcAddress(GetModuleHandle(L"dsound.dll"), "DirectSoundCreate");
    TrampHook32((BYTE*)oDirectSoundCreate, (BYTE*)hkDirectSoundCreate, 5);
    
    /// GetCurrentPositionDef oGetCurrentPosition = (GetCurrentPositionDef)GetProcAddress(GetModuleHandle(L"dsound.dll"), "GetCurrentPosition");
    /// TrampHook32((BYTE*)oGetCurrentPosition, (BYTE*)hkGetCurrentPosition, 5);

    fprintf(traceFile, "Hooked.\n");
}

void SetHook2(FILE* traceFile)
{
    if ((_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        fprintf(traceFile, "Hooked.\n");
    }
    else
    {
        fprintf(traceFile, "Error while hooking.\n");
    }
}

BOOL APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
    /* open file */
    FILE* file;
    fopen_s(&file, "D:\\Temp\\dlljnject.txt", "a+");

    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        fprintf(file, "DLL attach function called.\n");
        SetHook(file);
        /// SetHook(file);
        break;
    case DLL_PROCESS_DETACH:
        fprintf(file, "DLL detach function called.\n");
        break;
    case DLL_THREAD_ATTACH:
        fprintf(file, "DLL thread attach function called.\n");
        SetHook(file);
        // _hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0); 
        break;
    case DLL_THREAD_DETACH:
        fprintf(file, "DLL thread detach function called.\n");
        break;
    }

    /* close file */
    fclose(file);

    return TRUE;
}