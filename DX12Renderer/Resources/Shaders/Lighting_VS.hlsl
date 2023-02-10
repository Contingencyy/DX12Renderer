#include "DataStructs.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	matrix Model : MODEL;
	uint MaterialID : MATERIAL_ID;
};

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
ConstantBuffer<SceneData> SceneDataCB : register(b1);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float4 WorldPosition : WORLD_POSITION;
	uint MaterialID : MATERIAL_ID;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(IN.Model, float4(IN.Position, 1.0f));
	OUT.WorldPosition = OUT.Position;

	float4x4 jitterMatrix = SceneDataCB.ViewProjection;
	jitterMatrix[0][2] += GlobalCB.TAA_HaltonJitter.x;
	jitterMatrix[1][2] += GlobalCB.TAA_HaltonJitter.y;

	OUT.Position = mul(jitterMatrix, OUT.Position);

	OUT.TexCoord = IN.TexCoord;

	OUT.Normal = IN.Normal;
	OUT.Tangent = IN.Tangent;
	OUT.Bitangent = IN.Bitangent;

	OUT.MaterialID = IN.MaterialID;

	return OUT;
}