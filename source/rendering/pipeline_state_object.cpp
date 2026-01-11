#include "pch.hpp"
#include "rendering/pipeline_state_object.hpp"

using namespace slate;

void PipelineStateObject::Initialize(const D3D12_PIPELINE_STATE_STREAM_DESC& desc)
{
	auto device = App.Device().GetDevice();
	log::ThrowIfFailed(device->CreatePipelineState(&desc, IID_PPV_ARGS(&m_D3D12PipelineState)));
}

void PipelineStateObject::InitializeAsGraphicsPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	auto device = App.Device().GetDevice();

	log::ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_D3D12PipelineState)));
}
