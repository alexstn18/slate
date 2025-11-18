#pragma once
#include <string>
#include <wrl.h>
#include <d3dcompiler.h>
#include <d3d11.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace slate
{
	class Shader
	{
	public:
		bool CompileShader(const std::wstring& fileName, // wstring because D3DCompileFromFile expects UTF-8 string
			const std::string& entryPoint, // name of the function where shader begins execution
			const std::string& profile, // which version of HLSL we'll use (higher number = more features)
			ComPtr<ID3DBlob>& shaderBlob) const; // blob means fancy buffer

		bool CreateVertexShader(const std::wstring& fileName, ComPtr<ID3DBlob>& vertexShaderBlob) const;
		bool CreatePixelShader(const std::wstring& fileName, ComPtr<ID3DBlob>& pixelShaderBlob) const;
		[[nodiscard]] ComPtr<ID3D11InputLayout> GetInputLayout() const noexcept { return m_vertexLayout; }
		[[nodiscard]] ComPtr<ID3D11VertexShader> GetVertexShader() const noexcept { return m_vertexShader; }
		[[nodiscard]] ComPtr<ID3D11PixelShader> GetPixelShader() const noexcept { return m_pixelShader; }
	private:
		ComPtr<ID3D11InputLayout> m_vertexLayout{ nullptr };
		ComPtr<ID3D11VertexShader> m_vertexShader{ nullptr };
		ComPtr<ID3D11PixelShader> m_pixelShader{ nullptr };
	};
}
