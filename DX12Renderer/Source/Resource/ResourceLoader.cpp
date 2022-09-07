#include "Pch.h"
#include "Resource/ResourceLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tinygltf/tiny_gltf.h>

ImageInfo ResourceLoader::LoadImage(const std::string& filepath)
{
	ImageInfo imageInfo = {};
	imageInfo.Data = stbi_load(filepath.c_str(), &imageInfo.Width, &imageInfo.Height, &imageInfo.ChannelsPerPixel, STBI_rgb_alpha);

	if (imageInfo.Data == nullptr)
	{
		ASSERT(true, "Failed to load texture");
	}

	return imageInfo;
}

std::string ResourceLoader::LoadShader(const std::string& filepath)
{
	std::string shaderCode = "";
	std::ifstream shaderFile(filepath);

	if (shaderFile.is_open())
	{
		std::string line;
		while (std::getline(shaderFile, line))
		{
			shaderCode += line + "\n";
		}

		shaderFile.close();
	}
	else
	{
		LOG_ERR("Could not open shader file");
	}

	return shaderCode;
}

tinygltf::Model ResourceLoader::LoadGLTFModel(const std::string& filepath)
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

	ASSERT(result, "Failed to parse glTF model");
	return tinygltf;
}
