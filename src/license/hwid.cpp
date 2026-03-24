#include "hwid.h"
#include <windows.h>
#include <iphlpapi.h>
#include <bcrypt.h>
#include <vector>
#include <iomanip>
#include <sstream>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace {
    static std::string getMotherboardSerial() {
        std::string mbSerial = "UNKNOWN_MB";
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char value[256] = {0};
            DWORD value_length = sizeof(value) - 1;
            DWORD type;
            if (RegQueryValueExA(hKey, "BaseBoardSerialNumber", nullptr, &type, reinterpret_cast<LPBYTE>(&value), &value_length) == ERROR_SUCCESS) {
                mbSerial = std::string(value);
            }
            RegCloseKey(hKey);
        }
        return mbSerial;
    }

    static std::string getMacAddress() {
        std::string macAddress = "UNKNOWN_MAC";
        ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
        std::vector<uint8_t> buffer(outBufLen);
        PIP_ADAPTER_INFO pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());

        if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(outBufLen);
            pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());
        }

        if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
            if (pAdapterInfo) {
                char mac[18];
                snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                         pAdapterInfo->Address[0], pAdapterInfo->Address[1],
                         pAdapterInfo->Address[2], pAdapterInfo->Address[3],
                         pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
                macAddress = std::string(mac);
            }
        }
        return macAddress;
    }
}

std::string HWID::generate() {
    std::string mbSerial = getMotherboardSerial();
    std::string macAddress = getMacAddress();
    std::string combined = mbSerial + macAddress;

    // 4. SHA256 using BCrypt
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    DWORD cbHashObject = 0, cbResult = 0, cbHash = 0;
    std::vector<uint8_t> pbHashObject;
    std::vector<uint8_t> pbHash;
    std::string resultHash;

    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) {
        if (NT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbHashObject), sizeof(DWORD), &cbResult, 0))) {
            pbHashObject.resize(cbHashObject);

            if (NT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PBYTE>(&cbHash), sizeof(DWORD), &cbResult, 0))) {
                pbHash.resize(cbHash);

                if (NT_SUCCESS(BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, nullptr, 0, 0))) {
                    if (NT_SUCCESS(BCryptHashData(hHash, reinterpret_cast<PUCHAR>(const_cast<char*>(combined.data())), static_cast<ULONG>(combined.size()), 0))) {
                        if (NT_SUCCESS(BCryptFinishHash(hHash, pbHash.data(), cbHash, 0))) {
                            std::ostringstream oss;
                            oss << std::hex << std::setfill('0');
                            for (uint8_t byte : pbHash) {
                                oss << std::setw(2) << static_cast<int>(byte);
                            }
                            resultHash = oss.str();
                        }
                    }
                    BCryptDestroyHash(hHash);
                }
            }
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    return resultHash;
}
