struct VertexShaderInput
{
	float3 Position : POSITION;
	matrix Model : MODEL;
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
	lightViewSpacePos = mul(IN.Model, lightViewSpacePos);
	lightViewSpacePos = mul(LightMatrixCB.LightViewProjection, lightViewSpacePos);

	return lightViewSpacePos;
}