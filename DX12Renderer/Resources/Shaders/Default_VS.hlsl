struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	matrix Model : MODEL;
	float4 Color : COLOR;
};

struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumDirectionalLights;
	uint NumPointLights;
	uint NumSpotLights;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float4 WorldPosition : WORLD_POSITION;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(IN.Model, float4(IN.Position, 1.0f));
	OUT.WorldPosition = OUT.Position;
	OUT.Position = mul(SceneDataCB.ViewProjection, OUT.Position);

	OUT.TexCoord = IN.TexCoord;
	OUT.Normal = IN.Normal;
	OUT.Color = IN.Color;

	return OUT;
}