#include "pch.hpp"
#include "rendering/fence.hpp"

using namespace slate;

void Fence::Initialize(uint64_t initialValue)
{
	CreateFence(initialValue);
	CreateEventHandle();
	m_FenceValue = initialValue;
}

void Fence::Cleanup()
{
	if (m_FenceEvent)
	{
		::CloseHandle(m_FenceEvent);
		m_FenceEvent = 0;
	}

	m_Fence.Reset();
	m_FenceValue = 0;
}

uint64_t Fence::Signal(ComPtr<ID3D12CommandQueue> commandQueue)
{
	uint64_t fenceValueForSignal = ++m_FenceValue;
	log::ThrowIfFailed(commandQueue->Signal(m_Fence.Get(), fenceValueForSignal));
	return fenceValueForSignal;
}

void Fence::WaitForValue(uint64_t value, std::chrono::milliseconds duration)
{
	if (m_Fence->GetCompletedValue() < value)
	{
		log::ThrowIfFailed(m_Fence->SetEventOnCompletion(value, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void Fence::Flush(ComPtr<ID3D12CommandQueue> commandQueue)
{
	uint64_t fenceValueForSignal = Signal(commandQueue);
	WaitForValue(fenceValueForSignal);
}

uint64_t Fence::GetCompletedValue() const
{
	return m_Fence->GetCompletedValue();
}

bool Fence::IsComplete(uint64_t value) const
{
	return GetCompletedValue() >= value;
}

void Fence::CreateFence(uint64_t initialValue)
{
	auto device = App.Device().GetDevice();

	ComPtr<ID3D12Fence> fence{ nullptr };
	log::ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
}

void Fence::CreateEventHandle()
{
	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_FenceEvent && "Failed to create fence event");
}
