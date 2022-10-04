#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;

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
	Mesh(const std::vector<std::shared_ptr<Texture>>& textures, std::size_t startVertex, std::size_t startIndex, std::size_t numIndices,
		const glm::vec3& minBounds, const glm::vec3& maxBounds, const std::string& name, std::size_t hash);
	~Mesh();

	std::shared_ptr<Texture> GetTexture(MeshTextureType type) const;
	std::shared_ptr<Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
	void SetVertexBuffer(const std::shared_ptr<Buffer>& vertexBuffer) { m_VertexBuffer = vertexBuffer; }
	std::shared_ptr<Buffer> GetIndexBuffer() const { return m_IndexBuffer; }
	void SetIndexBuffer(const std::shared_ptr<Buffer>& indexBuffer) { m_IndexBuffer = indexBuffer; }

	std::size_t GetStartVertex() const { return m_StartVertex; }
	std::size_t GetStartIndex() const { return m_StartIndex; }
	std::size_t GetNumIndices() const { return m_NumIndices; }

	const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }
	const BoundingSphere& GetBoundingSphere() const { return m_BoundingSphere; }

	const std::string& GetName() const;
	std::size_t GetHash() const;

private:
	void CreateBoundingBox(const glm::vec3& minBounds, const glm::vec3& maxBounds);
	void CreateBoundingSphere(const glm::vec3& minBounds, const glm::vec3& maxBounds);

private:
	std::string m_Name = "Unnamed";
	std::size_t m_Hash = 0;

	std::shared_ptr<Buffer> m_VertexBuffer;
	std::shared_ptr<Buffer> m_IndexBuffer;
	std::size_t m_StartVertex = 0;
	std::size_t m_StartIndex = 0;
	std::size_t m_NumIndices = 0;

	std::shared_ptr<Texture> m_Textures[MeshTextureType::NUM_TEXTURE_TYPES];

	BoundingBox m_BoundingBox;
	BoundingSphere m_BoundingSphere;

};
