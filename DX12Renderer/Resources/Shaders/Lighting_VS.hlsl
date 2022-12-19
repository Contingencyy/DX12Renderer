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

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3x3 TBN : TBN;
	float4 WorldPosition : WORLD_POSITION;
	uint BaseColorTexture : BASE_COLOR_TEXTURE;
	uint NormalTexture : NORMAL_TEXTURE;
	uint MetallicRoughnessTexture : METALLIC_ROUGHNESS_TEXTURE;
	float Metalness : METALNESS_FACTOR;
	float Roughness : ROUGHNESS_FACTOR;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(IN.Model, float4(IN.Position, 1.0f));
	OUT.WorldPosition = OUT.Position;
	OUT.Position = mul(SceneDataCB.ViewProjection, OUT.Position);
	OUT.TexCoord = IN.TexCoord;

	float3 T = normalize(mul(IN.Model, float4(IN.Tangent, 0.0f))).xyz;
	float3 B = normalize(mul(IN.Model, float4(IN.Bitangent, 0.0f))).xyz;
	float3 N = mul(IN.Model, float4(IN.Normal, 0.0f));

	OUT.TBN = transpose(float3x3(T, B, N));

	OUT.BaseColorTexture = IN.BaseColorTexture;
	OUT.NormalTexture = IN.NormalTexture;
	OUT.MetallicRoughnessTexture = IN.MetallicRoughnessTexture;
	OUT.Metalness = IN.Metalness;
	OUT.Roughness = IN.Roughness;

	return OUT;
}