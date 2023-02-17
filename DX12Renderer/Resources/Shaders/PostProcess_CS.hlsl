#include "DataStructs.hlsl"

float3 LinearToneMapping(float3 color, float exposure, float gamma)
{
	color = clamp(exposure * color, 0.0f, 1.0f);
	color = pow(abs(color), (1.0f / gamma));

	return color;
}

float3 ReinhardToneMapping(float3 color, float exposure, float gamma)
{
	color = float3(1.0f, 1.0f, 1.0f) - exp(-color * exposure);
	color = pow(abs(color), 1.0f / gamma);

	return color;
}

float3 Uncharted2ToneMapping(float3 color, float exposure, float gamma)
{
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;
	float W = 11.2f;

	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(abs(color), (1.0f / gamma));

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

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
Texture2D SourceTexture : register(t0, space0);
RWTexture2D<float4> SDRColorTarget : register(u0, space0);

#define LINEAR_TONEMAP 0
#define REINHARD_TONEMAP 1
#define UNCHARTED2_TONEMAP 2
#define FILMIC_TONEMAP 3
#define ACES_FILMIC_TONEMAP 4

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float exposure = GlobalCB.TM_Exposure;
	float gamma = GlobalCB.TM_Gamma;

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	// Transform texture data depending on which debug texture mode is currently enabled
	switch (GlobalCB.PP_DebugShowTextureMode)
	{
	case DebugShowTextureMode_Default:
	{
		finalColor = SourceTexture[threadID.xy].rgba;
	} break;
	case DebugShowTextureMode_Velocity:
	{
		float2 velocity = SourceTexture[threadID.xy].rg;
		finalColor = float3(abs(velocity), 0.0f);
	} break;
	}

	// ---------------------------------------------------------------
	// Apply tone mapping to the final SDR color target

	switch (GlobalCB.TM_Type)
	{
	case LINEAR_TONEMAP:
		SDRColorTarget[threadID.xy].rgb = LinearToneMapping(finalColor.rgb, exposure, gamma);
		break;
	case REINHARD_TONEMAP:
		SDRColorTarget[threadID.xy].rgb = ReinhardToneMapping(finalColor.rgb, exposure, gamma);
		break;
	case UNCHARTED2_TONEMAP:
		SDRColorTarget[threadID.xy].rgb = Uncharted2ToneMapping(finalColor.rgb, exposure, gamma);
		break;
	case FILMIC_TONEMAP:
		SDRColorTarget[threadID.xy].rgb = FilmicToneMapping(finalColor.rgb);
		break;
	case ACES_FILMIC_TONEMAP:
		SDRColorTarget[threadID.xy].rgb = ACESFilmicToneMapping(finalColor.rgb);
		break;
	}
}