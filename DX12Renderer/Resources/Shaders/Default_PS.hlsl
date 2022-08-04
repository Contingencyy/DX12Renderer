Texture2D tex2D : register(t0);
Texture2D norm2D : register(t1);
SamplerState samp2D : register(s0);

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float4 WorldPosition : WORLD_POSITION;
};

struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumPointlights;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);

struct Pointlight
{
	float3 Position;
	float Range;
	float3 Attenuation;
	float4 Diffuse;
	float4 Ambient;
};

struct PointlightBuffer
{
	Pointlight pointlights[100];
};

ConstantBuffer<PointlightBuffer> PointlightCB : register(b2);

float3 PointLightAttenuation(float3 fragPos, float3 fragNormal, float3 diffuseColor, Pointlight pointlight);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float4 diffuseColor = IN.Color * tex2D.Sample(samp2D, IN.TexCoord);
	float4 textureNormal = norm2D.Sample(samp2D, IN.TexCoord);
	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	for (uint i = 0; i < SceneDataCB.NumPointlights; ++i)
	{
		finalColor += float4(PointLightAttenuation(float3(IN.WorldPosition.xyz), normalize(IN.Normal * textureNormal.xyz), diffuseColor.xyz, PointlightCB.pointlights[i]), diffuseColor.a);
	}

	return finalColor;
}

float3 PointLightAttenuation(float3 fragPos, float3 fragNormal, float3 diffuseColor, Pointlight pointlight)
{
	float3 ldirection = pointlight.Position - fragPos;
	float distance = length(ldirection);

	ldirection /= distance;
	float diff = max(dot(fragNormal, ldirection), 0.0f);

	//float3 reflectDirection = reflect(-ldirection, fragNormal);
	//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

	float attenuation = 1.0f / (pointlight.Attenuation.x + (pointlight.Attenuation.y * distance) + (pointlight.Attenuation.z * (distance * distance)));
	
	float3 ambient = pointlight.Ambient.xyz * diffuseColor;
	float3 diffuse = pointlight.Diffuse.xyz * diff * diffuseColor;
	//float3 specular = pointlight.Specular.xyz * spec * specularColor;

	ambient *= attenuation;
	diffuse *= attenuation;
	//specular *= attenuation;

	return (ambient + diffuse /*+ specular*/);
}
