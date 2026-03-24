#include "license.h"
#include "hwid.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <vector>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

namespace {
    // Valid license key - accept "12345678" as a valid key
    static const std::string VALID_KEY = "12345678";

    std::string getLicensePath() {
        char path[MAX_PATH];
        if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
            std::string fullPath(path);
            size_t pos = fullPath.find_last_of("\\/");
            if (pos != std::string::npos) {
                return fullPath.substr(0, pos + 1) + "license.key";
            }
        }
        return "license.key";
    }

    std::string g_LicenseKeyFromDialog;

    INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_COMMAND) {
            if (LOWORD(wParam) == IDOK) {
                char buf[512] = {0};
                GetDlgItemTextA(hwndDlg, 100, buf, sizeof(buf));
                g_LicenseKeyFromDialog = buf;
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
        } else if (uMsg == WM_CLOSE) {
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE;
    }

    WORD* AlignWord(WORD* p) {
        return (WORD*)(((ULONG_PTR)p + 3) & ~3);
    }
    
    bool promptLicenseKey(std::string& outKey) {
        g_LicenseKeyFromDialog.clear();
        
        HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT, 1024);
        if (!hMem) {
            std::cerr << "[ERROR] Failed to allocate memory for license dialog\n";
            return false;
        }
        
        LPDLGTEMPLATE pDlg = (LPDLGTEMPLATE)GlobalLock(hMem);
        if (!pDlg) {
            GlobalFree(hMem);
            std::cerr << "[ERROR] Failed to lock memory for license dialog\n";
            return false;
        }
        
        pDlg->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_CENTER;
        pDlg->cdit = 4;
        pDlg->x = 0; pDlg->y = 0; pDlg->cx = 200; pDlg->cy = 100;
        
        WORD* pWord = (WORD*)(pDlg + 1);
        *pWord++ = 0; // menu
        *pWord++ = 0; // class
        
        const wchar_t* title = L"License Activation";
        while (*title) *pWord++ = *title++;
        *pWord++ = 0;
        
        // Static text
        pWord = AlignWord(pWord);
        LPDLGITEMTEMPLATE pItem = (LPDLGITEMTEMPLATE)pWord;
        pItem->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
        pItem->x = 10; pItem->y = 10; pItem->cx = 180; pItem->cy = 15;
        pItem->id = -1;
        pWord = (WORD*)(pItem + 1);
        *pWord++ = 0xFFFF; *pWord++ = 0x0082; 
        const wchar_t* text = L"Enter your license key:";
        while (*text) *pWord++ = *text++;
        *pWord++ = 0; 
        *pWord++ = 0; 
        
        // Edit control
        pWord = AlignWord(pWord);
        pItem = (LPDLGITEMTEMPLATE)pWord;
        pItem->style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP;
        pItem->x = 10; pItem->y = 30; pItem->cx = 180; pItem->cy = 15;
        pItem->id = 100;
        pWord = (WORD*)(pItem + 1);
        *pWord++ = 0xFFFF; *pWord++ = 0x0081; 
        *pWord++ = 0; 
        *pWord++ = 0; 
        
        // OK Button
        pWord = AlignWord(pWord);
        pItem = (LPDLGITEMTEMPLATE)pWord;
        pItem->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP;
        pItem->x = 40; pItem->y = 60; pItem->cx = 50; pItem->cy = 15;
        pItem->id = IDOK;
        pWord = (WORD*)(pItem + 1);
        *pWord++ = 0xFFFF; *pWord++ = 0x0080; 
        const wchar_t* btnOk = L"OK";
        while (*btnOk) *pWord++ = *btnOk++;
        *pWord++ = 0;
        *pWord++ = 0;
        
        // Cancel Button
        pWord = AlignWord(pWord);
        pItem = (LPDLGITEMTEMPLATE)pWord;
        pItem->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
        pItem->x = 110; pItem->y = 60; pItem->cx = 50; pItem->cy = 15;
        pItem->id = IDCANCEL;
        pWord = (WORD*)(pItem + 1);
        *pWord++ = 0xFFFF; *pWord++ = 0x0080; 
        const wchar_t* btnCancel = L"Cancel";
        while (*btnCancel) *pWord++ = *btnCancel++;
        *pWord++ = 0;
        *pWord++ = 0;
        
        GlobalUnlock(hMem);
        
        INT_PTR ret = DialogBoxIndirectA(GetModuleHandleA(NULL), (LPDLGTEMPLATE)hMem, NULL, DialogProc);
        GlobalFree(hMem);
        
        if (ret == IDOK && !g_LicenseKeyFromDialog.empty()) {
            outKey = g_LicenseKeyFromDialog;
            return true;
        }
        return false;
    }
}

void License::saveKey(const std::string& key) {
    std::ofstream file(getLicensePath(), std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << key;
    }
}

std::string License::loadKey() {
    std::string key;
    std::ifstream file(getLicensePath());
    if (file.is_open()) {
        std::getline(file, key);
    }
    return key;
}

bool License::validate() {
    std::string key = loadKey();
    if (key.empty()) {
        if (!promptLicenseKey(key)) {
            return false;
        }
        saveKey(key);
    }

    // Check if key matches valid hardcoded key
    if (key == VALID_KEY) {
        return true;
    }

    // Fallback to server validation for other keys
    std::string hwid = HWID::generate();
    std::string jsonBody = "{\"key\": \"" + key + "\", \"hwid\": \"" + hwid + "\"}";

    bool isValid = false;

    HINTERNET hSession = WinHttpOpen(L"CS2 Overlay License Checker", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        HINTERNET hConnect = WinHttpConnect(hSession, L"YOUR_SERVER", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/check", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                LPCWSTR header = L"Content-Type: application/json\r\n";
                if (WinHttpSendRequest(hRequest, header, -1L, (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.length(), (DWORD)jsonBody.length(), 0)) {
                    if (WinHttpReceiveResponse(hRequest, nullptr)) {
                        DWORD dwSize = 0;
                        DWORD dwDownloaded = 0;
                        std::string response;
                        
                        do {
                            dwSize = 0;
                            if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                                std::vector<char> buffer(dwSize + 1, 0);
                                if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                                    response.append(buffer.data(), dwDownloaded);
                                }
                            }
                        } while (dwSize > 0);

                        if (response.find("ok") != std::string::npos) {
                            isValid = true;
                        }
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }

    return isValid;
}
