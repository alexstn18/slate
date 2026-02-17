#pragma once

#include "d3d12/include/d3d12.h"
#include "d3d12/include/d3dx12.h"

#include <dxgi1_6.h>
#include <wrl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <chrono>

struct Vertex
{
	glm::vec3 Position;
	glm::vec4 Color;
};

namespace slate
{
	class Device
	{
	public:
		Device();
		~Device();

		[[nodiscard]] ComPtr<ID3D12Device2> GetDevice() const { return m_Device; }
		void CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	private:
		ComPtr<ID3D12Device2> m_Device{ nullptr };
	};
}