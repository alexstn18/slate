#include "pch.hpp"
#include "rendering/command_queue.hpp"
#include "rendering/fence.hpp"

using namespace slate;

// Inspired by:
// https://github.com/PappaNiels/IntroDXR/blob/main/code/DXRCore/Renderer/Attributes/CommandQueue.cpp

CommandQueue::~CommandQueue()
{
	delete m_Fence;
}

void CommandQueue::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	auto device = App.Device().GetDevice();

	log::ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
	m_Fence = new Fence();
	m_Fence->Initialize(0);
	log::Info("Command queue created successfully");
}

uint64_t CommandQueue::ExecuteCommandLists(const std::vector<ID3D12GraphicsCommandList*> cmdList)
{
	for (auto cmd : cmdList)
	{
		cmd->Close();
	}

	ID3D12CommandList* const* cmdListsRaw = reinterpret_cast<ID3D12CommandList* const*>(cmdList.data());

	m_CommandQueue->ExecuteCommandLists(static_cast<uint32_t>(cmdList.size()), cmdListsRaw);

	return m_Fence->Signal(m_CommandQueue);
}

void CommandQueue::WaitForFenceValue(uint64_t value)
{
	m_Fence->WaitForValue(value);
}

void CommandQueue::Flush()
{
	m_Fence->Flush(m_CommandQueue);
}
