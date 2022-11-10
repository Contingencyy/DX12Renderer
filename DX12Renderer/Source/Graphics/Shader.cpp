#include "Pch.h"
#include "Graphics/Shader.h"
#include "Resource/FileLoader.h"

Shader::Shader(const std::wstring& filepath, const std::string& entryPoint, const std::string& target)
{
    Compile(filepath, entryPoint, target);
}

Shader::~Shader()
{
}

void Shader::Compile(const std::wstring& filepath, const std::string& entryPoint, const std::string& target)
{
    std::vector<LPCWSTR> args;
    //args.emplace_back(DXC_ARG_SKIP_VALIDATION);

#ifdef _DEBUG
    args.emplace_back(DXC_ARG_DEBUG);
    args.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#endif

    ComPtr<IDxcLibrary> library;
    DX_CALL(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

    ComPtr<IDxcCompiler> compiler;
    DX_CALL(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    uint32_t codePage = CP_UTF8;
    ComPtr<IDxcBlobEncoding> sourceBlob;
    DX_CALL(library->CreateBlobFromFile(filepath.c_str(), &codePage, &sourceBlob));

    ComPtr<IDxcUtils> utils;
    DX_CALL(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

    ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
    DX_CALL(utils->CreateDefaultIncludeHandler(&defaultIncludeHandler));

    ComPtr<IDxcOperationResult> result;

    HRESULT hr = compiler->Compile(
        sourceBlob.Get(),
        filepath.c_str(),
        StringHelper::StringToWString(entryPoint).c_str(),
        StringHelper::StringToWString(target).c_str(),
        args.data(), static_cast<uint32_t>(args.size()),
        NULL, 0,
        defaultIncludeHandler.Get(),
        &result
    );

    if (!SUCCEEDED(hr))
    {
        if (result)
        {
            ComPtr<IDxcBlobEncoding> errorBlob;
            hr = result->GetErrorBuffer(&errorBlob);

            if (SUCCEEDED(hr) && errorBlob)
            {
                LOG_ERR(static_cast<const char*>(errorBlob->GetBufferPointer()));
                errorBlob->Release();
            }
        }
    }
    else
    {
        result->GetStatus(&hr);
    }

    result->GetResult(&m_ShaderByteBlob);

    m_ShaderByteCode.pShaderBytecode = m_ShaderByteBlob->GetBufferPointer();
    m_ShaderByteCode.BytecodeLength = m_ShaderByteBlob->GetBufferSize();
}
