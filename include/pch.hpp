#pragma once

// stl
#include <memory>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <chrono>

// windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <Windows.h>

// dx
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "d3d12/include/d3d12.h"
#include "d3d12/include/d3dx12.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#include "log.hpp"
#include "application.hpp"
#include "device.hpp"