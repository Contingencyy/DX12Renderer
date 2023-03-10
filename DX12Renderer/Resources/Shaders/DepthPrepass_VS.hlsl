#include "Common.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float4x4 Transform : TRANSFORM;
};

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
ConstantBuffer<SceneData> SceneDataCB : register(b1);

float4 main(VertexShaderInput IN) : SV_POSITION
{
	float4x4 jitterMatrix = SceneDataCB.ViewProjection;
	jitterMatrix[0][2] += GlobalCB.TAA_HaltonJitter.x;
	jitterMatrix[1][2] += GlobalCB.TAA_HaltonJitter.y;

	float4 OutPosition = mul(IN.Transform, float4(IN.Position, 1.0f));
	OutPosition = mul(jitterMatrix, OutPosition);

	return OutPosition;
}