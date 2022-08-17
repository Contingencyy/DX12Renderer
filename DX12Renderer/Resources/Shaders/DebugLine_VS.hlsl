struct VertexShaderInput
{
	float3 Position : POSITION;
	float4 Color : COLOR;
};

struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumPointlights;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(SceneDataCB.ViewProjection, float4(IN.Position, 1.0f));
	OUT.Color = IN.Color;

	return OUT;
}