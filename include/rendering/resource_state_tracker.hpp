#pragma once

#include <map>
#include <mutex>
#include <unordered_map>

// source:
// https://github.com/jpvanoosten/LearningDirectX12/blob/main/DX12Lib/src/ResourceStateTracker.cpp

namespace slate
{
	class Resource;
	class CommandList;

	class ResourceStateTracker
	{
	public:
		ResourceStateTracker();
		virtual ~ResourceStateTracker();

		// Push a resource barrier to the resource state tracker
		// (transition, UAV or alias)
		void ResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);

		// Push a transition resource barrier to the resource state tracker
		// By default, all subresources of a resource are transitioned to the same state
		// It doesn't need to know the "before" state of the transition barrier
		// because it's able to resolve the "before" state of the (sub)resource before 
		// the resource barrier is added to the command list
		void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		void TransitionResource(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	
		// Push a UAV resource barrier to the given resource
		void UAVBarrier(const Resource* resource = nullptr);
		// Push an aliasing barrier for the given resource
		// Either the beforeResource or afterResource can be null which indicates
		// that any placed or reserved resource could cause aliasing
		void AliasBarrier(const Resource* resourceBefore = nullptr, const Resource* resourceAfter = nullptr);

		// Flush any pending resource barriers to the command list
		u32 FlushPendingResourceBarriers(const std::shared_ptr<CommandList>& commandList);

		// Flush any (non-pending) resource barriers that have been pushed to the 
		// resource state tracker
		void FlushResourceBarriers(const std::shared_ptr <CommandList>& commandList);
		
		// Commit final resource states to the global resource state map
		// This must be called when the command list is closed
		void CommitFinalResourceStates();

		// Reset state tracking. This must be done when the command list is reset
		void Reset();

		// The global state must be locked before plushing pending resource barriers
		// and committing the final resource state to the global resource state
		// This ensures consistency of the global resource state between command list
		// executions
		static void Lock();

		// Unlocks the global resource state after the final states have been committed
		// to the global resource state array
		static void Unlock();

		// Add a resource with a given state to the global resource array (map)
		// This should be done when the resource is created for the first time
		static void AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

		// Remove a resource from the global resource state array (map)
		// This should only be done when the resource is destroyed
		static void RemoveGlobalResourceState(ID3D12Resource* resource);
	private:
		// dynamic array of resource barriers
		using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;

		// Pending resource transitions are committed before a command list
		// is executed on the command queue. This guarantees that resources will
		// be in the expected state at the beginning of a command list
		ResourceBarriers m_PendingResourceBarriers{};

		// Resource barriers that need to be committed to the command list
		ResourceBarriers m_ResourceBarriers{};

		struct ResourceState {
			// Set a subresource to a particular state
			void SetSubResourceState(UINT subResource, D3D12_RESOURCE_STATES state)
			{
				if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
					State = state;
					SubResourceState.clear();
				}
				else {
					SubResourceState[subResource] = state;
				}
			}

			// Get the state of a (sub)resource within the resource
			// If the specified subresource is not found in the SubResourceState
			// then the state of the resource (D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			// is returned
			D3D12_RESOURCE_STATES GetSubResourceState(UINT subResource) const {
				D3D12_RESOURCE_STATES state = State;

				const auto iter = SubResourceState.find(subResource);
				if (iter != SubResourceState.end()) {
					state = iter->second;
				}

				return state;
			}

			// If the SubResourceState array(map) is empty, then the State var
			// defines the state of all the subresources
			D3D12_RESOURCE_STATES State;
			std::map<UINT, D3D12_RESOURCE_STATES> SubResourceState;
		};

		// maps a resource (ptr) to its ResourceState
		using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;

		// The final (last known state) of the resources within a command list 
		// The final resource state is committed to the global resource state when the
		// command list is closed but before it is executed on the command queue
		ResourceStateMap m_FinalResourceState;

		// The global resource state array(map) stores the state of a resource between
		// command list execution
		static ResourceStateMap ms_GlobalResourceState;

		// The mutex protects shared access to the GlobalResourceState map
		static std::mutex ms_GlobalMutex;
		static bool ms_IsLocked;
	};
}

