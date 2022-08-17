struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumPointlights;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	return IN.Color;
}