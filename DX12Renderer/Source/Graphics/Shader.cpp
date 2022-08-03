#include "Pch.h"
#include "Graphics/Shader.h"
#include "Resource/ResourceLoader.h"

Shader::Shader(const std::wstring& filepath, const std::string& entryPoint, const std::string& target)
{
    Compile(filepath, entryPoint, target);
}

Shader::~Shader()
{
}

void Shader::Compile(const std::wstring& filepath, const std::string& entryPoint, const std::string& target)
{
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;

    std::string filepathStr = StringHelper::WStringToString(filepath);
    std::string shaderCode = ResourceLoader::LoadShader(filepathStr);

    HRESULT hr = D3DCompile(&shaderCode[0], shaderCode.length(), filepathStr.c_str(), nullptr, nullptr,
        entryPoint.c_str(), target.c_str(), compileFlags, 0, &m_ShaderBlob, &errorBlob);

    if (!SUCCEEDED(hr) || errorBlob)
    {
        LOG_ERR(static_cast<const char*>(errorBlob->GetBufferPointer()));
        errorBlob->Release();
    }

    m_ShaderByteCode = CD3DX12_SHADER_BYTECODE(m_ShaderBlob.Get());
}
