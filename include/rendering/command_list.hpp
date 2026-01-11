#pragma once

namespace slate
{
	class Resource;
	class Buffer;
	class UploadBuffer;
	class ResourceStateTracker;
	class DynamicDescriptorHeap;
	class PipelineStateObject;
	class RenderTarget;
	class VertexBuffer;
	class IndexBuffer;

	// ID3D12CommandList wrapper
	// Handles resource barriers, copying CPU and GPU resources, texture loading, mipmap-gen, binding resources to the pipeline, descriptor heaps, draw and dispatch cmds
	class CommandList : public std::enable_shared_from_this<CommandList>
	{
	public:
		CommandList();
		virtual ~CommandList();
		void Initialize(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	
		void Reset(ComPtr<ID3D12CommandAllocator> allocator);
		bool Close(const std::shared_ptr<CommandList>& pendingCommandList);
		void Close();

		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList> Get() const noexcept { return m_CommandList; }
		[[nodiscard]] ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const noexcept { return m_CommandAllocator; }
		[[nodiscard]] D3D12_COMMAND_LIST_TYPE GetType() const noexcept { return m_CommandListType; }

		// Used to forward a D3D12_RESORUCE_TRANSITION_BARRIER structure to ResourceStateTracker::ResourceBarrier
		void TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		void TransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		
		void UAVBarrier(const Resource& resource, bool flushBarriers);

		void AliasingBarrier(const Resource& beforeRes, const Resource& afterRes, bool flushBarriers);

		void FlushResourceBarriers();
		void TrackObject(ComPtr<ID3D12Object> object);
		void ReleaseTrackedObjects();
		void TrackResource(ComPtr<ID3D12Object> object);
		void TrackResource(const std::shared_ptr<Resource>& res);
		void TrackResource(const Resource& res);

		// Used to copy one GPU resource to another (copying of resources is a common operation in rendering pipelines)
		void CopyResource(Resource& dstRes, const Resource& srcRes);
		
		void ResolveSubResource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes, u32 dstSubResource = 0u, u32 srcSubResource = 0u);
		void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
		void SetPipelineState(const std::shared_ptr<PipelineStateObject>& pipelineState);
		void SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
		void SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
		void SetGraphicsRootSignature(const std::shared_ptr<RootSignature>& rootSignature);
		void SetComputeRootSignature(const std::shared_ptr<RootSignature>& rootSignature);
		void SetViewport(const D3D12_VIEWPORT& viewport);
		void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);
		void SetScissorRect(const D3D12_RECT& scissorRect);
		void SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);
		void SetRenderTarget(const RenderTarget& renderTarget);
		void SetVertexBuffer(u32 slot, const VertexBuffer& vertexBuffer);
		void SetIndexBuffer(const IndexBuffer& indexBuffer);
		void Dispatch(u32 numGroupsX, u32 numGroupsY = 1u, u32 numGroupsZ = 1u);

		void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float clearColor[4]);
		void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, u8 stencil = 0);

		void UploadBufferData(Buffer& buffer, const void* data, size_t sizeInBytes);

		// Uses UploadBuffer class to update a constant buffer that needs to change often (e.g. world matrix for a model)
		void SetGraphicsDynamicConstantBuffer(u32 rootParameterIndex, size_t sizeInBytes, const void* bufferData);
		
		// Uses DynamicDescriptorHeap class to stage an SRV to a GPU-visible descriptor heap
		// This method also transitions the resource to the correct state for use as an SRV on the graphics or compute pipelines
		void SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset, const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource, UINT numSubResources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv);

		// Used to render geometry to the currently-bound render target
		// Before executing a "Draw" command on the command list, all resource barriers must be flushed to the command list
		// using the "FlushResourceBarriers" method and any resource descriptors that were staged to the DynamicDescriptorHeap
		// need to be committed
		void Draw(u32 vertexCount, u32 instanceCount = 1u, u32 startVertex = 0u, u32 startInstance = 0u); 

		void DrawIndexed(u32 indexCount, u32 instanceCount, u32 startIndex, i32 baseVertex, u32 startInstance);
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
	
		ID3D12PipelineState* m_PipelineState{ nullptr };
		ID3D12RootSignature* m_RootSignature{ nullptr };

		TrackedObjects m_TrackedObjects{};
	};
}

