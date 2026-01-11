#include "pch.hpp"
#include "rendering/descriptor_allocation.hpp"
#include "rendering/descriptor_allocator_page.hpp"

using namespace slate;

DescriptorAllocation::DescriptorAllocation()
	: m_Descriptor{0}
	, m_NumHandles{0u}
	, m_DescriptorSize{0u}
	, m_Page{nullptr}
{}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, u32 numHandles, u32 descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page)
	: m_Descriptor{descriptor}
	, m_NumHandles{numHandles}
	, m_DescriptorSize{descriptorSize}
	, m_Page{page}
{}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
	: m_Descriptor{allocation.m_Descriptor}
	, m_NumHandles{allocation.m_NumHandles}
	, m_DescriptorSize{allocation.m_DescriptorSize}
	, m_Page{std::move(allocation.m_Page)}
{
	allocation.m_Descriptor.ptr = 0;
	allocation.m_NumHandles = 0;
	allocation.m_DescriptorSize = 0;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other) noexcept
{
	// Free this descriptor if it points to anything
	Free();

	m_Descriptor = other.m_Descriptor;
	m_NumHandles = other.m_NumHandles;
	m_DescriptorSize = other.m_DescriptorSize;
	m_Page = std::move(other.m_Page);

	other.m_Descriptor.ptr = 0;
	other.m_NumHandles = 0;
	other.m_DescriptorSize = 0;

	return *this;
}

bool DescriptorAllocation::IsNull() const
{
	return m_Descriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(u32 offset) const
{
	assert(offset < m_NumHandles);
	return { m_Descriptor.ptr + (m_DescriptorSize * offset) };
}

u32 DescriptorAllocation::GetNumHandles() const
{
	return m_NumHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
	return m_Page;
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && m_Page)
	{
		m_Page->Free(std::move(*this), static_cast<u64>(App.Renderer().GetFrameCount()));

		m_Descriptor.ptr = 0;
		m_NumHandles = 0;
		m_DescriptorSize = 0;
		m_Page.reset();
	}
}
