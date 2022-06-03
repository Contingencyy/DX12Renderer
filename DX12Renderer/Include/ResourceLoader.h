#pragma once
#include <tinygltf/tiny_gltf.h>

struct ImageInfo
{
	unsigned char* Data;

	int Width;
	int Height;
	int ChannelsPerPixel;
};

class ResourceLoader
{
public:
	static ImageInfo LoadImage(const std::string& filepath);
	static std::string LoadShader(const std::string& filepath);
	static tinygltf::Model LoadModel(const std::string& filepath);
};
