#include "pch.hpp"
#include "rendering/render_target.hpp"

using namespace slate;

void RenderTarget::SetRenderTargetViews(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvs)
{
	m_RTVs = rtvs;
}

void RenderTarget::SetRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
	m_RTVs.clear();
	m_RTVs.push_back( rtv );
}

void RenderTarget::SetDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
	m_DSV = dsv;
}

void RenderTarget::Reset()
{
	m_RTVs.clear();
	m_DSV.ptr = 0;
}
