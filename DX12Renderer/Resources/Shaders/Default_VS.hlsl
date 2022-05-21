struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	matrix Model : MODEL;
	float3 Color : COLOR;
};

struct ViewProjection
{
	matrix ViewProjection;
};

ConstantBuffer<ViewProjection> ViewProjectionCB : register(b0);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Color : COLOR;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(IN.Model, float4(IN.Position, 1.0f));
	OUT.Position = mul(ViewProjectionCB.ViewProjection, OUT.Position);
	OUT.TexCoord = IN.TexCoord;
	OUT.Color = IN.Color;

	return OUT;
}