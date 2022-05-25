Texture2D tex2D : register(t0);
SamplerState samp2D : register(s0);

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
};

float4 main(PixelShaderInput IN) : SV_TARGET
{
	return IN.Color * tex2D.Sample(samp2D, IN.TexCoord);
}