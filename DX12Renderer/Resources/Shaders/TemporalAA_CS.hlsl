#include "DataStructs.hlsl"

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
Texture2D HDRColorTarget : register(t0, space0);
Texture2D DepthTarget : register(t0, space1);
Texture2D TAAHistory : register(t0, space2);
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

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	int2 currentPos = threadID.xy;

	float3 sourceColor = HDRColorTarget[currentPos].rgb;
	float3 historyColor = max(0, TAAHistory[currentPos].rgb);

	float sourceWeight = GlobalCB.TAA_SourceWeight;
	float historyWeight = 1.0f - sourceWeight;

	float3 taaResult = sourceColor * sourceWeight + historyColor * historyWeight;

	TAAResolveTarget[currentPos] = float4(taaResult, 1.0f);
}
