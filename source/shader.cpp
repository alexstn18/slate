#include "shader.hpp"
#include "log.hpp"
#include "application.hpp"
#include "device.hpp"

using namespace slate;

bool Shader::CompileShader(const std::wstring& fileName, 
    const std::string& entryPoint, 
    const std::string& profile, 
    ComPtr<ID3DBlob>& shaderBlob) const
{
    // disables legacy syntax support and makes the shader compiler "stricter"
    constexpr UINT compileFlags{ D3DCOMPILE_ENABLE_STRICTNESS };

    ComPtr<ID3DBlob> tempShaderBlob{ nullptr };
    ComPtr<ID3DBlob> errorBlob{ nullptr };

    if (FAILED(D3DCompileFromFile(fileName.data(), nullptr, // pDefines (nullptr here) an array of macros that we may want to define 
        D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.data(), // pInclude (macro arg) is a pointer to a ID3DInclude object which is used to specify #include directives in shaders
        profile.data(), compileFlags, 0, &tempShaderBlob, &errorBlob)))
    {
        log::Error("Failed to read shader from file");
        if (errorBlob != nullptr)
        {
            log::Error("With error message: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    shaderBlob = std::move(tempShaderBlob);

    return true;
}

bool Shader::CreateVertexShader(const std::wstring& fileName, ComPtr<ID3DBlob>& vertexShaderBlob) const
{
    if (!CompileShader(fileName, "main", "vs_5_0", vertexShaderBlob)) // Main shader entry point, vs_5_0 = Vertex Shader Model 5.0
    {
        return false;
    }

    ComPtr<ID3D11VertexShader> vertexShader = nullptr;
    if (FAILED(App.Device().GetDevice()->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader)))
    {
        log::Error("Failed to compile vertex shader from path");
        return false;
    }

    //m_vertexShader = vertexShader;

    return true;
}

bool Shader::CreatePixelShader(const std::wstring& fileName, ComPtr<ID3DBlob>& pixelShaderBlob) const
{
    if (!CompileShader(fileName, "main", "ps_5_0", pixelShaderBlob)) // Main shader entry point, vs_5_0 = Pixel Shader Model 5.0
    {
        return false;
    }

    ComPtr<ID3D11PixelShader> pixelShader = nullptr;
    if (FAILED(App.Device().GetDevice()->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader)))
    {
        log::Error("Failed to compile pixel shader");
        return false;
    }

    return true;
}
