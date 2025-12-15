#pragma once

namespace slate
{
	class CommandList
	{
	public:
		void Initialize(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	
		void Reset();
		void Close();

		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList> Get() const { return m_CommandList; }
		[[nodiscard]] ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return m_CommandAllocator; }
	protected:
		friend class Device;
	private:
		ComPtr<ID3D12GraphicsCommandList> m_CommandList{ nullptr };
		ComPtr<ID3D12CommandAllocator> m_CommandAllocator{ nullptr };
	};
}

