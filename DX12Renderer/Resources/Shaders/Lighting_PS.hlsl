#include "DataStructs.hlsl"

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float4 WorldPosition : WORLD_POSITION;
	uint2 TexIndices : TEX_INDICES;
};

struct DirectionalLightBuffer
{
	DirectionalLight DirLight;
};

struct PointLightBuffer
{
	PointLight PointLights[50];
};

struct SpotLightBuffer
{
	SpotLight SpotLights[50];
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);
ConstantBuffer<DirectionalLightBuffer> DirLightCB : register(b1);
ConstantBuffer<PointLightBuffer> PointLightCB : register(b2);
ConstantBuffer<SpotLightBuffer> SpotLightCB : register(b3);

Texture2D Texture2DTable[] : register(t0, space0);
TextureCube TextureCubeTable[] : register(t0, space1);
SamplerState Samp2DWrap : register(s0, space0);
SamplerState Samp2DBorder : register(s0, space1);

float3 CalculateDirectionalLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, DirectionalLight dirLight);
float3 CalculatePointLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, PointLight pointLight);
float3 CalculateSpotLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, SpotLight spotLight);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float4 diffuseColor = IN.Color * Texture2DTable[IN.TexIndices.x].Sample(Samp2DWrap, IN.TexCoord);
	float4 textureNormal = Texture2DTable[IN.TexIndices.y].Sample(Samp2DWrap, IN.TexCoord);
	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	finalColor += diffuseColor.xyz * SceneDataCB.Ambient;

	float4 fragPosWS = IN.WorldPosition;
	float3 fragNormalWS = normalize(IN.Normal * textureNormal.xyz);

	if (SceneDataCB.NumDirectionalLights == 1)
		finalColor += CalculateDirectionalLight(fragPosWS, fragNormalWS, diffuseColor.xyz, DirLightCB.DirLight);

	for (uint p = 0; p < SceneDataCB.NumPointLights; ++p)
	{
		finalColor += CalculatePointLight(fragPosWS, fragNormalWS, diffuseColor.xyz, PointLightCB.PointLights[p]);
	}

	for (uint s = 0; s < SceneDataCB.NumSpotLights; ++s)
	{
		finalColor += CalculateSpotLight(fragPosWS, fragNormalWS, diffuseColor.xyz, SpotLightCB.SpotLights[s]);
	}

	return float4(finalColor, diffuseColor.w);
}

float DirectionToDepthValue(float3 direction, float near, float far)
{
	float3 absDir = abs(direction);
	// Get face of cubemap we will be sampling from
	float localZComp = max(absDir.x, max(absDir.y, absDir.z));

	// Transform depth into range of 0..1 with the corresponding near and far plane
	float normZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZComp;
	return (normZComp + 1.0f) * 0.5f;
}

float GetSlopeBias(float angle)
{
	return clamp(0.0000025f * tan(acos(angle)), 0.0f, 0.000005f);
}

float CalculateDirectionalShadow(float4 fragPosLS, float angle, uint shadowMapIndex)
{
	float3 projectedCoords = fragPosLS.xyz / fragPosLS.w;
	float currentDepth = projectedCoords.z;
	projectedCoords = projectedCoords * 0.5f;
	projectedCoords.xy += 0.5f;

	// Invert Y for D3D style screen coordinates
	projectedCoords.y = 1.0f - projectedCoords.y;

	float bias = GetSlopeBias(angle);
	float shadow = 0.0f;

	float closestDepth = Texture2DTable[shadowMapIndex].Sample(Samp2DBorder, projectedCoords.xy).r;
	shadow = currentDepth + bias < closestDepth ? 1.0f : 0.0f;

	return shadow;
}

float CalculatePointShadow(float3 lightToFrag, float angle, float farPlane, uint shadowMapIndex)
{
	float currentDepth = DirectionToDepthValue(lightToFrag, farPlane, 0.1f);
	float closestDepth = TextureCubeTable[shadowMapIndex].Sample(Samp2DBorder, lightToFrag).r;

	float bias = GetSlopeBias(angle);
	float shadow = currentDepth + bias < closestDepth ? 1.0f : 0.0f;

	return shadow;
}

float3 CalculateDirectionalLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, DirectionalLight dirLight)
{
	float3 ldirection = normalize(-dirLight.Direction);
	float angle = dot(fragNormalWS, ldirection);

	float4 fragPosLS = mul(dirLight.ViewProjection, fragPosWS);
	float shadow = CalculateDirectionalShadow(fragPosLS, angle, dirLight.ShadowMapIndex);

	//float3 reflectDirection = reflect(-ldirection, fragNormal);
	//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

	float3 ambient = dirLight.Ambient * diffuseColor;

	float diff = max(angle, 0.0f);
	float3 diffuse = dirLight.Diffuse * diff * diffuseColor;
	//float3 specular = pointlight.Specular.xyz * spec * specularColor;

	return (ambient + (1.0f - shadow) * (diffuse /*+ specular*/));
}

float3 CalculatePointLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, PointLight pointLight)
{
	float3 ldirection = pointLight.Position - fragPosWS.xyz;
	float distance = length(ldirection);

	float3 ambient = float3(0.0f, 0.0f, 0.0f);
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	//float3 specular = float3(0.0f, 0.0f, 0.0f);

	float shadow = 0.0f;

	if (distance <= pointLight.Range)
	{
		ldirection /= distance;

		// After calculating the distance attenuation, we need to rescale the value to account for the LIGHT_RANGE_EPSILON at which the light is cutoff/ignored
		float attenuation = clamp(1.0f / (pointLight.Attenuation.x + (pointLight.Attenuation.y * distance) + (pointLight.Attenuation.z * (distance * distance))), 0.0f, 1.0f);
		attenuation = (attenuation - 0.01f) / (1.0f - 0.01f);
		attenuation = max(attenuation, 0.0f);

		ambient = pointLight.Ambient * diffuseColor;
		ambient *= attenuation;

		float3 lightToFrag = (fragPosWS.xyz - pointLight.Position);
		float angle = dot(fragNormalWS, ldirection);
		shadow = CalculatePointShadow(lightToFrag, angle, pointLight.Range, pointLight.ShadowMapIndex);

		float diff = max(angle, 0.0f);
		diffuse = pointLight.Diffuse * diff * diffuseColor;

		//float3 reflectDirection = reflect(-ldirection, fragNormal);
		//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);
		//float3 specular = pointlight.Specular.xyz * spec * specularColor;

		diffuse *= attenuation;
		//specular *= attenuation;
	}

	return (ambient + (1.0f - shadow) * diffuse /*+ specular*/);
}

float3 CalculateSpotLight(float4 fragPosWS, float3 fragNormalWS, float3 diffuseColor, SpotLight spotLight)
{
	float3 ldirection = spotLight.Position - fragPosWS.xyz;
	float distance = length(ldirection);

	float3 ambient = float3(0.0f, 0.0f, 0.0f);
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	//float3 specular = float3(0.0f, 0.0f, 0.0f);

	float shadow = 0.0f;

	if (distance <= spotLight.Range)
	{
		ldirection /= distance;

		// After calculating the distance attenuation, we need to rescale the value to account for the LIGHT_RANGE_EPSILON at which the light is cutoff/ignored
		float attenuation = clamp(1.0f / (spotLight.Attenuation.x + (spotLight.Attenuation.y * distance) + (spotLight.Attenuation.z * (distance * distance))), 0.0f, 1.0f);
		attenuation = (attenuation - 0.01f) / (1.0f - 0.01f);
		attenuation = max(attenuation, 0.0f);

		ambient = spotLight.Ambient * diffuseColor;
		ambient *= attenuation;

		float angleToLight = dot(ldirection, -spotLight.Direction);

		if (angleToLight > spotLight.OuterConeAngle)
		{

			float4 fragPosLS = mul(spotLight.ViewProjection, fragPosWS);
			float angle = dot(fragNormalWS, ldirection);
			shadow = CalculateDirectionalShadow(fragPosLS, angle, spotLight.ShadowMapIndex);

			float diff = max(angle, 0.0f);
			diffuse = spotLight.Diffuse * diff * diffuseColor;

			//float3 reflectDirection = reflect(-ldirection, fragNormal);
			//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);
			//float3 specular = pointlight.Specular.xyz * spec * specularColor;

			float epsilon = abs(spotLight.InnerConeAngle - spotLight.OuterConeAngle);
			float coneAttenuation = clamp((angleToLight - spotLight.OuterConeAngle) / epsilon, 0.0f, 1.0f);

			diffuse *= coneAttenuation * attenuation;
			//specular *= coneAttenuation * attenuation;
		}
	}

	return (ambient + (1.0f - shadow) * diffuse /*+ specular*/);
}
