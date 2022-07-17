Texture2D tex2D : register(t0);
SamplerState samp2D : register(s0);

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

static const float GAMMA = 1.5f;

float3 LinearToneMapping(float3 color);
float3 ReinhardToneMapping(float3 color);
float3 FilmicToneMapping(float3 color);
float3 UnchartedTwoToneMapping(float3 color);
float3 ACESFilmicToneMapping(float3 color);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float4 sampled = tex2D.Sample(samp2D, IN.TexCoord);
	return float4(UnchartedTwoToneMapping(sampled.xyz), sampled.w);
}

float3 LinearToneMapping(float3 color)
{
	float exposure = 1.;
	color = clamp(exposure * color, 0.0f, 1.0f);
	color = pow(color, (1.0f / GAMMA));

	return color;
}

float3 ReinhardToneMapping(float3 color)
{
	float exposure = 1.5;
	color *= exposure / (1.0 + color / exposure);
	color = pow(color, (1.0 / GAMMA));

	return color;
}

float3 FilmicToneMapping(float3 color)
{
	color = max(float3(0.0f, 0.0f, 0.0f), color - float3(0.004f, 0.004f, 0.004f));
	color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);

	return color;
}

float3 UnchartedTwoToneMapping(float3 color)
{
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;
	float W = 11.2f;
	float exposure = 2.0f;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, (1.0f / GAMMA));
	return color;
}

float3 ACESFilmicToneMapping(float3 color)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}
