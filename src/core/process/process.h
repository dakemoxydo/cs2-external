#pragma once
#include <windows.h>
#include <string>

namespace Core {
    class Process {
    public:
        static bool Attach(const std::wstring& processName);
        static void Detach();
        static HANDLE GetHandle();
        static DWORD GetProcessId();

    private:
        static HANDLE hProcess;
        static DWORD processId;
    };
}
