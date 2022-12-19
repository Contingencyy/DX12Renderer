#include "DataStructs.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	matrix Model : MODEL;
	uint BaseColorTexture : BASE_COLOR_TEXTURE;
	uint NormalTexture : NORMAL_TEXTURE;
	uint MetallicRoughnessTexture : METALLIC_ROUGHNESS_TEXTURE;
	float Metalness : METALNESS_FACTOR;
	float Roughness : ROUGHNESS_FACTOR;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

float4 main(VertexShaderInput IN) : SV_POSITION
{
	float4 OutPosition = mul(IN.Model, float4(IN.Position, 1.0f));
	OutPosition = mul(SceneDataCB.ViewProjection, OutPosition);

	return OutPosition;
}