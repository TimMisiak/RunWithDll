#include <iostream>
#include <Windows.h>
#include <string>
#include <sstream>

BOOL InjectDLL(DWORD processID, const wchar_t* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::wcerr << L"OpenProcess failed." << std::endl;
        return FALSE;
    }

    LPVOID pDllPath = VirtualAllocEx(hProcess, 0, (wcslen(dllPath) + 1) * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
    if (pDllPath == NULL) {
        std::wcerr << L"VirtualAllocEx failed." << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, (wcslen(dllPath) + 1) * sizeof(wchar_t), 0);

    auto pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pDllPath, 0, nullptr);
    if (hThread == NULL) {
        std::wcerr << L"CreateRemoteThread failed." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}

std::wstring BuildCommandLine(int argc, wchar_t* argv[]) {
    std::wostringstream cmdLine;
    for (int i = 2; i < argc; ++i) {
        std::wstring arg = argv[i];
        // Check if the argument is already quoted.
        if (arg.front() != L'"' && arg.back() != L'"' && arg.find(L' ') != std::wstring::npos) {
            cmdLine << L'"' << arg << L'"';
        } else {
            cmdLine << arg;
        }
        if (i < argc - 1) {
            cmdLine << L' ';
        }
    }
    return cmdLine.str();
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcerr << L"Usage: " << argv[0] << L" <DLL Path> <Executable Path> [Executable Args]" << std::endl;
        return 1;
    }

    const wchar_t* dllPath = argv[1];
    std::wstring cmdLine = BuildCommandLine(argc, argv);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(NULL, (LPWSTR)cmdLine.c_str(), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi)) {
        std::wcerr << L"CreateProcess failed (" << GetLastError() << L")." << std::endl;
        return 1;
    }

    if (!InjectDLL(pi.dwProcessId, dllPath)) {
        std::wcerr << L"DLL injection failed." << std::endl;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }

    DebugActiveProcessStop(pi.dwProcessId);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::wcout << L"DLL injected successfully." << std::endl;
    return 0;
}
