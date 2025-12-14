#include "pch.hpp"

#if defined(_WIN32)
// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#2.-set-agility-sdk-parameters

extern "C"
{
    __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 711;
}

extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif