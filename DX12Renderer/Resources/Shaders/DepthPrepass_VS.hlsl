#include "DataStructs.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	matrix Model : MODEL;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

float4 main(VertexShaderInput IN) : SV_POSITION
{
	float4 OutPosition = mul(IN.Model, float4(IN.Position, 1.0f));
	OutPosition = mul(SceneDataCB.ViewProjection, OutPosition);

	return OutPosition;
}