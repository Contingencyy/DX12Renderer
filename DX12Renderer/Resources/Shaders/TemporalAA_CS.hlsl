#include "Common.hlsl"

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
Texture2D<float4> HDRColorTarget : register(t0, space0);
Texture2D<float> DepthTarget : register(t0, space1);
Texture2D<float2> VelocityTarget : register(t0, space2);
Texture2D<float2> VelocityTargetPrevious : register(t0, space3);
Texture2D<float4> TAAHistory : register(t0, space4);
RWTexture2D<float4> TAAResolveTarget : register(u0, space0);

SamplerState SampPointBorder : register(s0, space0);

float4 SampleBilinear(Texture2D<float4> tex, float2 pos)
{
	int2   posi = pos;
	float2 posf = frac(pos);

	int2 sample_positions[] = {
		posi + int2(0, 0), posi + int2(1, 0),
		posi + int2(0, 1), posi + int2(1, 1),
	};

	float sample_weights[] = {
		(1.0 - posf.x) * (1.0 - posf.y), posf.x * (1.0 - posf.y),
		(1.0 - posf.x) * posf.y,  posf.x * posf.y,
	};

	float4 color = 0.0;
	float  sum_w = 0.0;

	for (int i = 0; i < 4; i++)
	{
		int2  sample_pos = sample_positions[i];
		float w = sample_weights[i];

		float4 sample_color = tex[sample_pos];
		color += w * sample_color;
		sum_w += w;
	}

	if (sum_w > 0.00001)
	{
		color /= sum_w;
	}

	return color;
}

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	int2 currentPos_SS = threadID.xy;
	float2 current_UV = float2(currentPos_SS) / GlobalCB.RenderResolution;

	// Neighborhood min and max values to clamp the history to
	float3 neighborMin = 10000.0f;
	float3 neighborMax = -10000.0f;

	float closestDepth = 1.0f;
	int2 closestDepthPos_SS = int2(0, 0);

	// Sample a 3x3 neighborhood and check for nearest depth to fight disocclusion artifacts
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			int2 samplePos = currentPos_SS + int2(x, y);
			float3 sampledColor = max(0, HDRColorTarget[samplePos].rgb);

			neighborMin = min(neighborMin, sampledColor);
			neighborMax = max(neighborMax, sampledColor);

			float sampledDepth = DepthTarget[samplePos].r;
			if (sampledDepth < closestDepth)
			{
				closestDepth = sampledDepth;
				closestDepthPos_SS = samplePos;
			}
		}
	}

	// Get the velocity and apply it to the current UV to get the history UV
	float2 currentVelocity_UV = VelocityTarget[closestDepthPos_SS].rg;
	float2 history_UV = current_UV - currentVelocity_UV;

	// Get the source and history color
	float3 sourceColor = HDRColorTarget[currentPos_SS].rgb;
	float3 historyColor = max(0, SampleBilinear(TAAHistory, history_UV * GlobalCB.RenderResolution).xyz);
	//float3 historyColor = max(0, TAAHistory[history_UV * GlobalCB.RenderResolution].rgb);
	//float3 historyColor = TAAHistory.SampleLevel(SampPointBorder, history_UV, 0).rgb;

	if (GlobalCB.TAA_UseNeighborhoodClamp)
		historyColor = clamp(historyColor, neighborMin, neighborMax);

	// If the history position is out of bounds, use the source color instead
	if (any(history_UV < 0 || history_UV > 1.0f))
		historyColor = sourceColor;

	// Calculate the source and history weights
	float sourceWeight = GlobalCB.TAA_SourceWeight;
	float historyWeight = 1.0f - sourceWeight;

	// Calculate final taa resolve
	float3 taaResult = sourceColor * sourceWeight + historyColor * historyWeight;

	// Velocity rejection
	int2 previousVelocitySamplePos = currentVelocity_UV * GlobalCB.RenderResolution - closestDepthPos_SS;
	float2 previousVelocity_UV = max(0, VelocityTargetPrevious[previousVelocitySamplePos].rg);
	float velocityDiff = length(previousVelocity_UV - currentVelocity_UV);
	float velocityDisocclusion = saturate(velocityDiff * GlobalCB.TAA_VelocityRejectionStrength);

	// Apply velocity rejection
	if (GlobalCB.TAA_UseVelocityRejection)
	{
		if (GlobalCB.TAA_ShowVelocityDisocclusion)
			taaResult = float3(velocityDisocclusion, velocityDisocclusion, velocityDisocclusion);
		else
			taaResult = lerp(taaResult, sourceColor, velocityDisocclusion);
	}
	
	TAAResolveTarget[currentPos_SS] = float4(taaResult, 1.0f);
}
