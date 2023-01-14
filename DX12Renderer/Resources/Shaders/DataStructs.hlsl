struct SceneData
{
	matrix ViewProjection;
	float3 ViewPosition;
	uint NumDirectionalLights;
	uint NumPointLights;
	uint NumSpotLights;
};

struct DirectionalLight
{
	float3 Direction;
	float3 Ambient;
	float3 Color;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct PointLight
{
	float3 Position;
	float Range;
	float3 Attenuation;
	float3 Color;

	uint ShadowMapIndex;
};

struct SpotLight
{
	float3 Position;
	float3 Direction;
	float Range;
	float3 Attenuation;
	float InnerConeAngle;
	float OuterConeAngle;
	float3 Color;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct LightCBData
{
	DirectionalLight DirLight;
	SpotLight SpotLights[50];
	PointLight PointLights[50];
};

struct TonemapSettings
{
	uint HDRRenderTargetIndex;
	uint SDRRenderTargetIndex;
	float Exposure;
	float Gamma;
	uint Type;
};

struct Material
{
	uint AlbedoTextureIndex;
	uint NormalTextureIndex;
	uint MetallicRoughnessTextureIndex;
	float Metalness;
	float Roughness;
};

struct MaterialCBData
{
	Material materials[1000];
};
