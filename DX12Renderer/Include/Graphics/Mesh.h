#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;

enum MeshBufferAttributeType : uint32_t
{
	ATTRIB_POSITION,
	ATTRIB_TEX_COORD,
	ATTRIB_NORMAL,
	ATTRIB_INDEX,
	NUM_ATTRIBUTE_TYPES
};

enum MeshTextureType : uint32_t
{
	TEX_BASE_COLOR,
	TEX_NORMAL,
	NUM_TEXTURE_TYPES
};

class Mesh
{
public:
	struct BoundingBox
	{
		glm::vec3 Min;
		glm::vec3 Max;
	};

	struct BoundingSphere
	{
		glm::vec3 Position;
		float Radius;
	};

public:
	Mesh(const std::vector<std::shared_ptr<Buffer>>& buffers, const std::vector<std::shared_ptr<Texture>>& textures,
		const glm::vec3& minBounds, const glm::vec3& maxBounds, const std::string& name, std::size_t hash);
	~Mesh();

	std::shared_ptr<Buffer> GetBuffer(MeshBufferAttributeType type) const;
	std::shared_ptr<Texture> GetTexture(MeshTextureType type) const;

	const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }
	const BoundingSphere& GetBoundingSphere() const { return m_BoundingSphere; }

	const std::string& GetName() const;
	std::size_t GetHash() const;

private:
	void CreateBoundingBox(const glm::vec3& minBounds, const glm::vec3& maxBounds);
	void CreateBoundingSphere(const glm::vec3& minBounds, const glm::vec3& maxBounds);

private:
	std::string m_Name;
	std::size_t m_Hash;

	std::shared_ptr<Buffer> m_Buffers[MeshBufferAttributeType::NUM_ATTRIBUTE_TYPES];
	std::shared_ptr<Texture> m_Textures[MeshTextureType::NUM_TEXTURE_TYPES];

	BoundingBox m_BoundingBox;
	BoundingSphere m_BoundingSphere;

};
