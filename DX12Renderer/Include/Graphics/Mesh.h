#pragma once
#include "Scene/BoundingVolume.h"
#include "Graphics/Material.h"

class Buffer;
class Texture;

struct MeshPrimitive
{
	MeshPrimitive() = default;

	Material Material;
	BoundingBox BB;

	std::size_t VerticesStart = 0;
	std::size_t IndicesStart = 0;
	std::size_t NumIndices = 0;

	std::shared_ptr<Buffer> VertexBuffer;
	std::shared_ptr<Buffer> IndexBuffer;

	std::string Name = "Unnamed";

};

struct Mesh
{
	Mesh() = default;

	std::vector<MeshPrimitive> Primitives;
	std::string Name = "Unnamed";
	std::size_t Hash = 0;

};
