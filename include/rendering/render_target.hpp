#pragma once
namespace slate {

	class RenderTarget {
	public:
		RenderTarget() = default;
		~RenderTarget() = default;

		// Set RTVs
		void SetRenderTargetViews(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvs);
		void SetRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
	
		// Set DSV
		void SetDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv);

		// Get views
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& GetRenderTargetViews() const { return m_RTVs; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const { return m_DSV; }

		// Queries
		u32 GetNumRenderTargets() const { return static_cast<u32>(m_RTVs.size()); }
		bool HasDepthStencil() const { return m_DSV.ptr != 0; }

		// Reset
		void Reset();
	private:
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVs{};
		D3D12_CPU_DESCRIPTOR_HANDLE m_DSV{};
	};

}

