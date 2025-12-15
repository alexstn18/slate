#pragma once

namespace slate
{
	class Fence;

	class CommandQueue
	{
	public:
		~CommandQueue();
		void Initialize(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		uint64_t ExecuteCommandLists(const std::vector<ID3D12GraphicsCommandList*>& cmdList);

		[[nodiscard]] ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return m_CommandQueue; }
		Fence& GetFence() { return *m_Fence; }

		void WaitForFenceValue(uint64_t value);
		void Flush();
	protected:
		friend class Device;
	private:
		ComPtr<ID3D12CommandQueue> m_CommandQueue{ nullptr };
		Fence* m_Fence{ nullptr };
	};
}
