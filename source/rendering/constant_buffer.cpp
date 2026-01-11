#include "pch.hpp"
#include "rendering/buffer.hpp"
#include "rendering/constant_buffer.hpp"

using namespace slate;

ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource)
	: Buffer(resource)
{
	m_SizeInBytes = GetResourceDesc().Width;
}

ConstantBuffer::~ConstantBuffer()
{
}
