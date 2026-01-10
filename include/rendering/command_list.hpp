#pragma once

namespace slate
{
	class Resource;
	class UploadBuffer;
	class ResourceStateTracker;
	class DynamicDescriptorHeap;

	// ID3D12CommandList wrapper
	// Handles resource barriers, copying CPU and GPU resources, texture loading, mipmap-gen, binding resources to the pipeline, descriptor heaps, draw and dispatch cmds
	class CommandList
	{
	public:
		void Initialize(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	
		void Reset();
		void Close();

		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList> Get() const noexcept { return m_CommandList; }
		[[nodiscard]] ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const noexcept { return m_CommandAllocator; }
	
		// Used to forward a D3D12_RESORUCE_TRANSITION_BARRIER structure to ResourceStateTracker::ResourceBarrier
		void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		
		void UAVBarrier(const Resource& resource, bool flushBarriers);

		void AliasingBarrier(const Resource& beforeRes, const Resource& afterRes, bool flushBarriers);

		void FlushResourceBarriers();
		void TrackObject(ComPtr<ID3D12Object> object);
		void TrackResource(const Resource& res);

		// Used to copy one GPU resource to another (copying of resources is a common operation in rendering pipelines)
		void CopyResource(Resource& dstRes, const Resource& srcRes);
		
		// Uses UploadBuffer class to update a constant buffer that needs to change often (e.g. world matrix for a model)
		void SetGraphicsDynamicConstantBuffer(u32 rootParameterIndex, size_t sizeInBytes, const void* bufferData);
		
		// Uses DynamicDescriptorHeap class to stage an SRV to a GPU-visible descriptor heap
		// This method also transitions the resource to the correct state for use as an SRV on the graphics or compute pipelines
		void SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset, const Resource& resource,
			D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource, UINT numSubResources,
			const D3D12_SHADER_RESOURCE_VIEW_DESC* srv);

		// Used to render geometry to the currently-bound render target
		// Before executing a "Draw" command on the command list, all resource barriers must be flushed to the command list
		// using the "FlushResourceBarriers" method and any resource descriptors that were staged to the DynamicDescriptorHeap
		// need to be committed
		void Draw(u32 vertexCount, u32 instanceCount, u32 startVertex, u32 startInstance); 
	protected:
		friend class Device;
	private:
		D3D12_COMMAND_LIST_TYPE m_CommandListType{};
		ComPtr<ID3D12GraphicsCommandList> m_CommandList{ nullptr };
		ComPtr<ID3D12CommandAllocator> m_CommandAllocator{ nullptr };

		using TrackedObjects = std::vector<ComPtr<ID3D12Object>>;

		std::shared_ptr<CommandList> m_ComputeCommandList{ nullptr };

		// Resource created in an upload heap
		// Useful for drawing of dynamic geometry or for uploading constant buffer data that changes every draw call
		std::unique_ptr<UploadBuffer> m_UploadBuffer{ nullptr };

		// Resource state tracker is used by the command list to track (per command list) the current state of a resource
		// The resource state tracker also tracks the global state of a resource in order to minimize resource state transitions
		std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker{ nullptr };

		std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{ nullptr };
	
		TrackedObjects m_TrackedObjects{};
	};
}

