#include "Pch.h"
#include "ResourceLoader.h"

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
		Logger::Log("Could not open shader file", Logger::Severity::ERR);
	}

	return shaderCode;
}

//tinygltf::Model ResourceLoader::LoadModel(const std::string& filepath)
//{
//	tinygltf::Model model;
//	tinygltf::TinyGLTF loader;
//	std::string err;
//	std::string warn;
//
//	bool result = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
//	if (!warn.empty())
//		Logger::Log(warn, Logger::Severity::WARN);
//	if (!err.empty())
//		Logger::Log(err, Logger::Severity::ERR);
//
//	ASSERT(result, "Failed to parse glTF model");
//	return model;
//}
