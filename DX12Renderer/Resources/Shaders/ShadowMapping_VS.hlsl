struct VertexShaderInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	matrix Model : MODEL;
	float4 Color : COLOR;
	uint BaseColorTexture : BASE_COLOR_TEXTURE;
	uint NormalTexture : NORMAL_TEXTURE;
	uint MetallicRoughnessTexture : METALLIC_ROUGHNESS_TEXTURE;
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