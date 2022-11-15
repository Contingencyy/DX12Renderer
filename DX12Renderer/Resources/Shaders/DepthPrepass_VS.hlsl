#include "DataStructs.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	matrix Model : MODEL;
	float4 Color : COLOR;
	uint2 TexIndices : TEX_INDICES;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

float4 main(VertexShaderInput IN) : SV_POSITION
{
	float4 OutPosition = mul(IN.Model, float4(IN.Position, 1.0f));
	OutPosition = mul(SceneDataCB.ViewProjection, OutPosition);

	return OutPosition;
}