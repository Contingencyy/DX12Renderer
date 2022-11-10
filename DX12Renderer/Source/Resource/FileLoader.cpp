#include "Pch.h"
#include "Resource/FileLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tinygltf/tiny_gltf.h>

ImageInfo FileLoader::LoadImage(const std::string& filepath)
{
	ImageInfo imageInfo = {};
	imageInfo.Data = stbi_load(filepath.c_str(), &imageInfo.Width, &imageInfo.Height, &imageInfo.ChannelsPerPixel, STBI_rgb_alpha);

	if (imageInfo.Data == nullptr)
	{
		ASSERT(true, "Failed to load texture: " + filepath);
	}

	return imageInfo;
}

std::string FileLoader::LoadShader(const std::string& filepath)
{
	std::string shaderCode = "";
	std::ifstream shaderFile(filepath);

	if (shaderFile)
	{
		shaderFile.seekg(0, std::ios::end);
		shaderCode.resize(shaderFile.tellg());
		shaderFile.seekg(0, std::ios::beg);
		shaderFile.read(&shaderCode[0], shaderCode.size());
		
		shaderFile.close();
	}
	else
	{
		LOG_ERR("Could not open shader file: " + filepath);
	}

	return shaderCode;
}

tinygltf::Model FileLoader::LoadGLTFModel(const std::string& filepath)
{
	tinygltf::Model tinygltf;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool result = loader.LoadASCIIFromFile(&tinygltf, &err, &warn, filepath);
	if (!warn.empty())
		LOG_WARN(warn);
	if (!err.empty())
		LOG_ERR(err);

	ASSERT(result, "Failed to parse glTF model: " + filepath);
	return tinygltf;
}
