#pragma once

class Shader
{
public:
	Shader(const std::wstring& filepath, const std::string& entryPoint, const std::string& target);
	~Shader();

	D3D12_SHADER_BYTECODE GetShaderByteCode() const { return m_ShaderByteCode; }

private:
	void Compile(const std::wstring& filepath, const std::string& entryPoint, const std::string& target);

private:
	ComPtr<IDxcBlob> m_ShaderByteBlob;
	D3D12_SHADER_BYTECODE m_ShaderByteCode;

};
