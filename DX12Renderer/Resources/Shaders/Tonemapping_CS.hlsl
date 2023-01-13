#include "DataStructs.hlsl"

ConstantBuffer<TonemapSettings> TonemapCB : register(b0);
Texture2D Texture2DTable[] : register(t0, space0);
RWTexture2D<float4> RWTexture2DTable[] : register(u0, space0);

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

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{

}