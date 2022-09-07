#include "Pch.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

Mesh::Mesh(const std::vector<std::shared_ptr<Buffer>>& buffers, const std::vector<std::shared_ptr<Texture>>& textures,
	const glm::vec3& minBounds, const glm::vec3& maxBounds, const std::string& name, std::size_t hash)
	: m_Name(name), m_Hash(hash)
{
	for (uint32_t i = 0; i < MeshBufferAttributeType::NUM_ATTRIBUTE_TYPES; ++i)
	{
		m_Buffers[i] = buffers[i];
	}

	for (uint32_t i = 0; i < MeshTextureType::NUM_TEXTURE_TYPES; ++i)
	{
		m_Textures[i] = textures[i];
	}

	CreateBoundingBox(minBounds, maxBounds);
	CreateBoundingSphere(minBounds, maxBounds);
}

Mesh::~Mesh()
{
}

std::shared_ptr<Buffer> Mesh::GetBuffer(MeshBufferAttributeType type) const
{
	return m_Buffers[type];
}

std::shared_ptr<Texture> Mesh::GetTexture(MeshTextureType type) const
{
	return m_Textures[type];
}

const std::string& Mesh::GetName() const
{
	return m_Name;
}

std::size_t Mesh::GetHash() const
{
	return m_Hash;
}

void Mesh::CreateBoundingBox(const glm::vec3& minBounds, const glm::vec3& maxBounds)
{
	m_BoundingBox.Min.x = static_cast<float>(minBounds.x);
	m_BoundingBox.Min.y = static_cast<float>(minBounds.y);
	m_BoundingBox.Min.z = static_cast<float>(minBounds.z);

	m_BoundingBox.Max.x = static_cast<float>(maxBounds.x);
	m_BoundingBox.Max.y = static_cast<float>(maxBounds.y);
	m_BoundingBox.Max.z = static_cast<float>(maxBounds.z);
}

void Mesh::CreateBoundingSphere(const glm::vec3& minBounds, const glm::vec3& maxBounds)
{
	double xRadius = (maxBounds.x - minBounds.x) / 2;
	double yRadius = (maxBounds.y - minBounds.y) / 2;
	double zRadius = (maxBounds.z - minBounds.z) / 2;

	m_BoundingSphere.Position.x = static_cast<float>(maxBounds.x - xRadius);
	m_BoundingSphere.Position.y = static_cast<float>(maxBounds.y - yRadius);
	m_BoundingSphere.Position.z = static_cast<float>(maxBounds.z - zRadius);

	m_BoundingSphere.Radius = static_cast<float>(std::max(std::max(xRadius, yRadius), zRadius));
}
