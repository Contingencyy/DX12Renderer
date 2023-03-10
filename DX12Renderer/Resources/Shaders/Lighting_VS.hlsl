#include "Common.hlsl"

struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	matrix Transform : TRANSFORM;
	matrix PrevFrameTransform : PREV_FRAME_TRANSFORM;
	uint MaterialID : MATERIAL_ID;
};

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
ConstantBuffer<SceneData> SceneDataCB : register(b1);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float4 CurrentPosNoJitter : CURRENT_POS_NO_JITTER;
	float4 PreviousPosNoJitter : PREVIOUS_POS_NO_JITTER;
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

	// Calculate world position of vertex and store it in the vertex shader output
	OUT.Position = mul(IN.Transform, float4(IN.Position, 1.0f));
	OUT.WorldPosition = OUT.Position;

	// Apply jitter to the current vertex position
	float4x4 jitterMatrix = SceneDataCB.ViewProjection;
	jitterMatrix[0][2] += GlobalCB.TAA_HaltonJitter.x;
	jitterMatrix[1][2] += GlobalCB.TAA_HaltonJitter.y;

	OUT.Position = mul(jitterMatrix, OUT.Position);

	// Calculate current and previous non-jittered position for velocity
	OUT.CurrentPosNoJitter = mul(SceneDataCB.ViewProjection, OUT.WorldPosition);
	OUT.PreviousPosNoJitter = mul(IN.PrevFrameTransform, float4(IN.Position, 1.0f));
	OUT.PreviousPosNoJitter = mul(GlobalCB.PrevViewProj, OUT.PreviousPosNoJitter);

	OUT.TexCoord = IN.TexCoord;
	OUT.Normal = IN.Normal;
	OUT.Tangent = IN.Tangent;
	OUT.Bitangent = IN.Bitangent;
	OUT.MaterialID = IN.MaterialID;

	return OUT;
}