#include "DataStructs.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	matrix Model : MODEL;
	float4 Color : COLOR;
	uint BaseColorTexture : BASE_COLOR_TEXTURE;
	uint NormalTexture : NORMAL_TEXTURE;
	uint MetallicRoughnessTexture : METALLIC_ROUGHNESS_TEXTURE;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3x3 TBN : TBN;
	float4 Color : COLOR;
	float4 WorldPosition : WORLD_POSITION;
	uint BaseColorTexture : BASE_COLOR_TEXTURE;
	uint NormalTexture : NORMAL_TEXTURE;
	uint MetallicRoughnessTexture : METALLIC_ROUGHNESS_TEXTURE;
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

	OUT.Color = IN.Color;
	OUT.BaseColorTexture = IN.BaseColorTexture;
	OUT.NormalTexture = IN.NormalTexture;
	OUT.MetallicRoughnessTexture = IN.MetallicRoughnessTexture;

	return OUT;
}