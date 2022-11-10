#pragma once
#include <tinygltf/tiny_gltf.h>

class Model;

struct ImageInfo
{
	unsigned char* Data;

	int Width;
	int Height;
	int ChannelsPerPixel;
};

class FileLoader
{
public:
	static ImageInfo LoadImage(const std::string& filepath);
	static std::string LoadShader(const std::string& filepath);
	static tinygltf::Model LoadGLTFModel(const std::string& filepath);
};
