#pragma once
namespace slate {
	class Buffer;
	class ConstantBuffer : public Buffer {
	public:
		size_t GetSizeInBytes() const
		{
			return m_SizeInBytes;
		}
	protected:
		ConstantBuffer(ComPtr<ID3D12Resource> resource);
		virtual ~ConstantBuffer();
	private:
		size_t m_SizeInBytes;
	};
}

