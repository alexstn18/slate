#include "pch.hpp"

#if defined(_WIN32)
// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#2.-set-agility-sdk-parameters

extern "C"
{
    __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 614;
}

extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\d3d12\\";
}
#endif