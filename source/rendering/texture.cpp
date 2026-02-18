#include "pch.hpp"
#include "rendering/resource.hpp"
#include "rendering/texture.hpp"
#include "rendering/resource_state_tracker.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace slate;

Texture::Texture(const std::filesystem::path& path, const std::wstring& name)
{
	auto device = App.Renderer().D3D12Device();
	auto commandList = App.Renderer().D3D12CommandList();

	i32 width{}, height{}, components{};
	u8* textureBuffer = stbi_load(path.string().c_str(), &width, &height, &components, 4);

	if (!textureBuffer) {
		log::Critical("Could not load texture from path: {}", path.string());
	}

	m_ResourceDesc.MipLevels = 1;
	// @TODO: add function for DXGI_FORMAT checking
	m_ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_ResourceDesc.Width = width;
	m_ResourceDesc.Height = height;
	m_ResourceDesc.SampleDesc.Count = 1;
	m_ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	log::ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&m_ResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_D3D12Resource.ReleaseAndGetAddressOf())
	));

	UINT64 uploadSize{ 0ull };
	device->GetCopyableFootprints(
		&m_ResourceDesc,
		0, 1, 0,
		nullptr, nullptr, nullptr,
		&uploadSize
	);

	ComPtr<ID3D12Resource> uploadResource{ nullptr };
	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
	log::ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadResource.ReleaseAndGetAddressOf())
	));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = textureBuffer;
	textureData.RowPitch = width * 4;
	textureData.SlicePitch = height * width * 4;

	UpdateSubresources(
		commandList.Get(),
		m_D3D12Resource.Get(), 
		uploadResource.Get(), 
		0, 0, 1, 
		&textureData
	);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_D3D12Resource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	commandList->ResourceBarrier(1, &barrier);

	stbi_image_free(textureBuffer);

	ResourceStateTracker::AddGlobalResourceState(
		m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	SetName(name);

	App.Renderer().TrackUpload(uploadResource);
}
