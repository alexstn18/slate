#pragma once

namespace slate
{
	class Fence
	{
	public:
		~Fence() { Cleanup(); }
		void Initialize(uint64_t initialValue = 0);
		void Cleanup();

		uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue);

		void WaitForValue(uint64_t value, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
		void Flush(ComPtr<ID3D12CommandQueue> commandQueue);

		uint64_t GetCompletedValue() const;
		uint64_t GetCurrentValue() const { return m_FenceValue; }
		bool IsComplete(uint64_t value) const;

		[[nodiscard]] ComPtr<ID3D12Fence> Get() const { return m_Fence; }
	protected:
		friend class Device;
	private:
		void CreateFence(uint64_t initialValue);
		void CreateEventHandle();

		ComPtr<ID3D12Fence> m_Fence{ nullptr };
		HANDLE m_FenceEvent{};
		uint64_t m_FenceValue{ 0ull };
	};
}

