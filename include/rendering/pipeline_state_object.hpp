#pragma once

namespace slate
{
	class PipelineStateObject
	{
	public:
		PipelineStateObject() = default;
		void Initialize(const D3D12_PIPELINE_STATE_STREAM_DESC& desc);
		void InitializeAsGraphicsPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
		virtual ~PipelineStateObject() = default;

		[[nodiscard]] ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const noexcept { return m_D3D12PipelineState; }
	private:
		ComPtr<ID3D12PipelineState> m_D3D12PipelineState{ nullptr };
	};
}