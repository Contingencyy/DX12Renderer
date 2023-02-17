#include "Common.hlsl"

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
Texture2D<float4> HDRColorTarget : register(t0, space0);
Texture2D<float> DepthTarget : register(t0, space1);
Texture2D<float2> VelocityTarget : register(t0, space2);
Texture2D<float4> TAAHistory : register(t0, space3);
RWTexture2D<float4> TAAResolveTarget : register(u0, space0);

float4 SampleBilinear(Texture2D<float4> tex, float2 pos)
{
	// pos is in pixel coordinates

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

//float3 ScreenSpaceToWorldSpace(float2 screenSpace, float depth, float4x4 invView, float4x4 invProj)
//{
//	float2 uv = screenSpace / GlobalCB.RenderResolution;
//	float4 ndc = float4(
//		uv * 2.0f - 1.0f,
//		1.0f,
//		1.0f
//	);
//	ndc.y *= -1.0f;
//
//	float4x4 invViewProj = mul(invProj, invView);
//	float4 worldSpace = mul(invViewProj, ndc);
//	worldSpace /= worldSpace.w;
//
//	return worldSpace;
//}
//
//float2 WorldSpaceToScreenSpace(float3 worldSpace, float depth, float4x4 view, float4x4 proj)
//{
//	float4x4 viewProj = mul(proj, view);
//	float4 ndc = mul(viewProj, float4(worldSpace, 1.0f));
//	float2 uv = float2(ndc.xy / 2.0f + 1.0f);
//	uv.y *= -1.0f;
//	float2 screenSpace = uv * GlobalCB.RenderResolution;
//
//	return screenSpace;
//}

float2 Project(float4x4 proj, float3 posVS)
{
	float4 posCS = mul(proj, float4(posVS, 1.0f));
	float3 normalized = posCS.xyz / posCS.w;
	float2 result = 0.5f * normalized.xy + 0.5f;

	return result;
}

float3 Unproject(float4x4 invProj, float2 posSS, float depth)
{
	float4 posCS = float4(2.0f * posSS - 1.0f, 1.0f, 1.0f);
	float3 posVS = mul(invProj, posCS).xyz;

	return posVS * depth;
}

float ConvertToLinearDepth(float depth, float4x4 invProj)
{
	float4 ndc = float4(0.0f, 0.0f, depth, 1.0f);
	float4 view = mul(invProj, ndc);

	return view.z;// / view.w;
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

	// Sample a 3x3 neighborhood and do depth rejection to fight disocclusion artifacts
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
	float2 velocity_UV = (VelocityTarget[closestDepthPos_SS].rg) * float2(0.5f, -0.5f);
	float2 history_UV = current_UV - velocity_UV;

	// Get the source and history color
	float3 sourceColor = HDRColorTarget[currentPos_SS].rgb;
	float3 historyColor = max(0, TAAHistory[history_UV * GlobalCB.RenderResolution].rgb);

	if (GlobalCB.TAA_NeighborhoodClamping)
		historyColor = clamp(historyColor, neighborMin, neighborMax);

	// If the history position is out of bounds, use the source color instead
	/*if (any(historyPos_SS < 0 || historyPos_SS >= GlobalCB.RenderResolution))
		historyColor = sourceColor;*/
	if (any(history_UV < 0 || history_UV > 1.0f))
		historyColor = sourceColor;

	// Calculate the source and history weights
	float sourceWeight = GlobalCB.TAA_SourceWeight;
	float historyWeight = 1.0f - sourceWeight;

	// Calculate final taa resolve
	float3 taaResult = sourceColor * sourceWeight + historyColor * historyWeight;
	TAAResolveTarget[currentPos_SS] = float4(taaResult, 1.0f);
}
