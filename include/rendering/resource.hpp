#pragma once
namespace slate
{
	class Resource
	{
	public:
		ComPtr<ID3D12Resource> Get() const { return m_D3D12Resource; }
		D3D12_RESOURCE_DESC GetResourceDesc() const;

		void SetName(const std::wstring& name);
		const std::wstring& GetName() const { return m_ResourceName; }

		bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
		bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;
	protected:
		Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
		Resource(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);

		virtual ~Resource() = default;

		ComPtr<ID3D12Resource> m_D3D12Resource{ nullptr };
		D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport{};
		std::unique_ptr<D3D12_CLEAR_VALUE> m_D3D12ClearValue{ nullptr };
		std::wstring m_ResourceName{};
	private:
		void CheckFeatureSupport();
	};
}

