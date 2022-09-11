struct VertexShaderInput
{
	float3 Position : POSITION;
	float4 Color : COLOR;
};

struct ViewProjection
{
	matrix VP;
};

ConstantBuffer<ViewProjection> ViewProjectionCB : register(b0);

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(ViewProjectionCB.VP, float4(IN.Position, 1.0f));
	OUT.Color = IN.Color;

	return OUT;
}