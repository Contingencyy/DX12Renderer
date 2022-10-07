struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	matrix Model : MODEL;
	float4 Color : COLOR;
	uint2 TexIndices : TEX_INDICES;
};

struct LightMatrix
{
	matrix LightViewProjection;
};

ConstantBuffer<LightMatrix> LightMatrixCB : register(b0);

float4 main(VertexShaderInput IN) : SV_POSITION
{
	float4 lightViewSpacePos = float4(IN.Position, 1.0f);

	// Transform vertex into light view space
	lightViewSpacePos = mul(lightViewSpacePos, IN.Model);
	lightViewSpacePos = mul(lightViewSpacePos, transpose(LightMatrixCB.LightViewProjection));

	return lightViewSpacePos;
}