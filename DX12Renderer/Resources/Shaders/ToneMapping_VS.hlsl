struct VertexShaderInput
{
	float2 Position : POSITION;
	float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;
	OUT.Position = float4(IN.Position, 0.0f, 1.0f);
	OUT.TexCoord = IN.TexCoord;

	return OUT;
}