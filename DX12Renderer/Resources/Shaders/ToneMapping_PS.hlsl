#include "DataStructs.hlsl"

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

ConstantBuffer<TonemapSettings> TonemapCB : register(b0);

Texture2D Texture2DTable[] : register(t0, space0);
SamplerState samp2D : register(s0);

float3 LinearToneMapping(float3 color);
float3 ReinhardToneMapping(float3 color);
float3 UnchartedTwoToneMapping(float3 color);
float3 FilmicToneMapping(float3 color);
float3 ACESFilmicToneMapping(float3 color);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float4 sampled = Texture2DTable[TonemapCB.HDRTargetIndex].Sample(samp2D, IN.TexCoord);

	switch (TonemapCB.Type)
	{
	case 0:
		return float4(LinearToneMapping(sampled.xyz), sampled.w);
	case 1:
		return float4(ReinhardToneMapping(sampled.xyz), sampled.w);
	case 2:
		return float4(UnchartedTwoToneMapping(sampled.xyz), sampled.w);
	case 3:
		return float4(FilmicToneMapping(sampled.xyz), sampled.w);
	case 4:
		return float4(ACESFilmicToneMapping(sampled.xyz), sampled.w);
	default:
		return float4(ReinhardToneMapping(sampled.xyz), sampled.w);
	}
}

float3 LinearToneMapping(float3 color)
{
	color = clamp(TonemapCB.Exposure * color, 0.0f, 1.0f);
	color = pow(abs(color), (1.0f / TonemapCB.Gamma));

	return color;
}

float3 ReinhardToneMapping(float3 color)
{
	color = float3(1.0f, 1.0f, 1.0f) - exp(-color * TonemapCB.Exposure);
	color = pow(abs(color), 1.0f / TonemapCB.Gamma);

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

	color *= TonemapCB.Exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(abs(color), (1.0f / TonemapCB.Gamma));

	return color;
}

float3 FilmicToneMapping(float3 color)
{
	color = max(float3(0.0f, 0.0f, 0.0f), color - float3(0.004f, 0.004f, 0.004f));
	color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);

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
