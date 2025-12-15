#include "pch.hpp"
#include "rendering/command_list.hpp"

using namespace slate;

void CommandList::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
	auto device = App.Device().GetDevice();

	log::ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_CommandAllocator)));
	log::ThrowIfFailed(device->CreateCommandList(0, type, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
}

void CommandList::Reset()
{
	log::ThrowIfFailed(m_CommandAllocator->Reset());
	log::ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr)); // nullptr = Pipeline State Object, optional
}

void CommandList::Close()
{
	log::ThrowIfFailed(m_CommandList->Close());
}
