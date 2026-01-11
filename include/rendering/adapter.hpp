#pragma once

#include <filesystem>

namespace slate
{
	class Adapter
	{
	public:
		void Initialize(bool useWarp);

		[[nodiscard]] ComPtr<IDXGIAdapter4> GetAdapter() const noexcept { return m_Adapter; }
		[[nodiscard]] bool UsesWarp() const noexcept { return m_UseWarp; }
		// TODO: fix GetGPUName() to not use std::string
		[[nodiscard]] std::string GetGPUName() const noexcept { return std::filesystem::path(m_GPUName).string(); }
		[[nodiscard]] i32 GetVRAMAsMB() const noexcept { return m_GPUMemoryInMB; }
	private:
		ComPtr<IDXGIAdapter4> m_Adapter{ nullptr };

		bool m_UseWarp{ false };

		std::wstring m_GPUName{};
		i32 m_GPUMemoryInMB{};
	};
}