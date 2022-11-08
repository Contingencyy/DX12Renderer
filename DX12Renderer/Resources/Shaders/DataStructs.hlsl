struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumDirectionalLights;
	uint NumPointLights;
	uint NumSpotLights;
};

struct DirectionalLight
{
	float3 Direction;
	float3 Ambient;
	float3 Diffuse;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct PointLight
{
	float3 Position;
	float Range;
	float3 Attenuation;
	float3 Ambient;
	float3 Diffuse;

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
	float3 Ambient;
	float3 Diffuse;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct TonemapSettings
{
	uint HDRTargetIndex;
	float Exposure;
	float Gamma;
	uint Type;
};