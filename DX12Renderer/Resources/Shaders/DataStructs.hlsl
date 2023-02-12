#define PI 3.14159265
#define INV_PI 1.0 / PI

static const float4x4 IDENTITY_MATRIX = float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

struct GlobalConstantBufferData
{
	// General render settings
	float4x4 ViewProj;
	float4x4 InvViewProj;

	float4x4 PrevViewProj;
	float4x4 PrevInvViewProj;

	uint2 RenderResolution;

	// TAA settings
	float2 TAA_HaltonJitter;
	float TAA_SourceWeight;
	uint TAA_NeighborhoodClamping;

	// Tonemapping settings
	float TM_Exposure;
	float TM_Gamma;
	uint TM_Type;
};

struct SceneData
{
	float4x4 ViewProjection;
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

	float4x4 ViewProjection;
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

	float4x4 ViewProjection;
	uint ShadowMapIndex;
};

struct LightCBData
{
	DirectionalLight DirLight;
	SpotLight SpotLights[50];
	PointLight PointLights[50];
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
	Material materials[500];
};
