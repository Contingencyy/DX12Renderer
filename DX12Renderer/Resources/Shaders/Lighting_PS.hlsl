#include "DataStructs.hlsl"

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3x3 TBN : TBN;
	float4 WorldPosition : WORLD_POSITION;
	uint MaterialID : MATERIAL_ID;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);
ConstantBuffer<MaterialCBData> MaterialCB : register(b1);
ConstantBuffer<LightCBData> LightCB : register(b2);

Texture2D Texture2DTable[] : register(t0, space0);
TextureCube TextureCubeTable[] : register(t0, space1);
SamplerState Sampler_Antisotropic_Wrap : register(s0, space0);
SamplerComparisonState Sampler_PCF : register(s0, space1);
SamplerState Sampler_Point_Wrap : register(s0, space2);

float3 CalculateDirectionalLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, DirectionalLight dirLight);
float3 CalculateSpotLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, SpotLight spotLight);
float3 CalculatePointLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, PointLight pointLight);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	Material mat = MaterialCB.materials[IN.MaterialID];

	float4 albedo = Texture2DTable[mat.AlbedoTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float3 textureNormal = Texture2DTable[mat.NormalTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord).xyz;
	textureNormal = (textureNormal * 2.0f) - 1.0f;

	float4 metallicRoughness = Texture2DTable[mat.MetallicRoughnessTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float metalness = metallicRoughness.b * mat.Metalness;
	float roughness = metallicRoughness.g * mat.Roughness;

	float4 fragPosWS = IN.WorldPosition;
	float3 fragNormalWS = normalize(mul(IN.TBN, textureNormal));
	float3 viewDir = normalize(SceneDataCB.ViewPosition - fragPosWS);

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	if (SceneDataCB.NumDirectionalLights == 1)
		finalColor += CalculateDirectionalLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.DirLight);

	for (uint s = 0; s < SceneDataCB.NumSpotLights; ++s)
	{
		finalColor += CalculateSpotLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.SpotLights[s]);
	}

	for (uint p = 0; p < SceneDataCB.NumPointLights; ++p)
	{
		finalColor += CalculatePointLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.PointLights[p]);
	}

	return float4(finalColor, albedo.w);
}

#define PI 3.14159265

// D - GGX normal distribution function
float NormalDist_GGX(float NoH, float a)
{
	float a2 = a * a;
	float f = (NoH * a2 - NoH) * NoH + 1.0f;
	return a2 / (PI * f * f);
}

// G - Smith-GGX height correlated visibility function
float SmithHeightCorrelated_GGX(float NoV, float NoL, float a)
{
	float a2 = a * a;
	float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
	float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
	return 0.5f / (GGXV + GGXL);
}

// F - Schlick fresnel function (optimized)
float3 SchlickFresnel(float u, float3 f0)
{
	return f0 + (1.0f - f0) * pow(1.0f - u, 5.0f);
}

// kD - Lambertian diffuse function
float DiffuseLambert()
{
	return 1.0f / PI;
}

//
//// F - Schlick fresnel function
//float SchlickFresnel(float u, float f0, float f90)
//{
//	return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
//}
//
//// kD - Disney's Burley diffuse BRDF
//float DiffuseBurley(float NoV, float NoL, float LoH, float roughness)
//{
//	float f90 = 0.5f + 2.0f * roughness * LoH * LoH;
//	float lightScatter = SchlickFresnel(NoL, 1.0f, f90);
//	float viewScatter = SchlickFresnel(NoV, 1.0f, f90);
//	return lightScatter * viewScatter * (1.0f / PI);
//}
//

struct BRDFResult
{
	float3 Diffuse;
	float3 Specular;
};

BRDFResult CalculateBRDF(float3 viewDir, float3 lightDir, float NoL, float3 albedo, float3 normal, float metalness, float roughness)
{
	float3 f0 = float3(0.04f, 0.04f, 0.04f);
	f0 = lerp(f0, albedo, metalness);

	float3 halfVec = normalize(viewDir + lightDir);

	float NoV = abs(dot(normal, viewDir)) + 1e-5;
	float NoH = clamp(dot(normal, halfVec), 0.0f, 1.0f);
	float LoH = clamp(dot(lightDir, halfVec), 0.0f, 1.0f);

	float D = NormalDist_GGX(NoH, roughness);
	float3 F = SchlickFresnel(LoH, f0);
	float V = SmithHeightCorrelated_GGX(NoV, NoL, roughness);

	BRDFResult result;
	result.Specular = (D * V) * F;
	result.Diffuse = ((1.0f - metalness) * albedo) * DiffuseLambert();

	return result;
}

float DistanceAttenuation(float3 lightAttenuation, float distance)
{
	// After calculating the distance attenuation, we need to rescale the value to account for the LIGHT_RANGE_EPSILON at which the light is cutoff/ignored
	float attenuation = clamp(1.0f / (lightAttenuation.x + (lightAttenuation.y * distance) + (lightAttenuation.z * (distance * distance))), 0.0f, 1.0f);
	attenuation = (attenuation - 0.01f) / (1.0f - 0.01f);
	return max(attenuation, 0.0f);
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

#define PCF_SAMPLE_RANGE 2

float CalculateDirectionalShadow(float4 fragPosLS, float angle, uint shadowMapIndex)
{
	float3 projectedCoords = fragPosLS.xyz / fragPosLS.w;
	float currentDepth = projectedCoords.z;
	projectedCoords = projectedCoords * 0.5f;
	projectedCoords.xy += 0.5f;

	// Invert Y for D3D style screen coordinates
	projectedCoords.y = 1.0f - projectedCoords.y;
	float shadow = 0.0f;

	if (projectedCoords.z > 1.0f || projectedCoords.z < 0.0f)
	{
		shadow = 1.0f;
	}
	else
	{
		// Sample from -PCF_SAMPLE_RANGE to +PCF_SAMPLE_RANGE texels with a bilinear filter to obtain a multisampled and bilinearly filtered shadow
		[unroll]
		for (int x = -PCF_SAMPLE_RANGE; x <= PCF_SAMPLE_RANGE; ++x)
		{
			[unroll]
			for (int y = -PCF_SAMPLE_RANGE; y <= PCF_SAMPLE_RANGE; ++y)
			{
				shadow += Texture2DTable[shadowMapIndex].SampleCmpLevelZero(Sampler_PCF, projectedCoords.xy, currentDepth, int2(x, y));
			}
		}
		shadow /= (PCF_SAMPLE_RANGE * 2 + 1) * (PCF_SAMPLE_RANGE * 2 + 1);
	}

	return shadow;
}

float CalculateOmnidirectionalShadow(float3 lightToFrag, float angle, float farPlane, uint shadowMapIndex)
{
	float currentDepth = DirectionToDepthValue(lightToFrag, farPlane, 0.1f);
	float shadow = 0.0f;

	if (currentDepth > 1.0f || currentDepth < 0.0f)
	{
		shadow = 1.0f;
	}
	else
	{
		float2 textureSize;
		TextureCubeTable[shadowMapIndex].GetDimensions(textureSize.x, textureSize.y);
		float texelSize = 1.0f / textureSize.x;

		// Sample from -PCF_SAMPLE_RANGE to +PCF_SAMPLE_RANGE texels with a bilinear filter to obtain a multisampled and bilinearly filtered shadow
		// For cubemaps we need to calculate the offset of texels manually since cube maps do not support an offset parameter by default.
		[unroll]
		for (int x = -PCF_SAMPLE_RANGE; x <= PCF_SAMPLE_RANGE; ++x)
		{
			[unroll]
			for (int y = -PCF_SAMPLE_RANGE; y <= PCF_SAMPLE_RANGE; ++y)
			{
				float3 uvwOffset = (float3(x, y, 0.0f) * length(lightToFrag)) * texelSize;
				float3 uvwSample = lightToFrag.xyz + uvwOffset;
				shadow += TextureCubeTable[shadowMapIndex].SampleCmpLevelZero(Sampler_PCF, uvwSample.xyz, currentDepth);
			}
		}
		shadow /= (PCF_SAMPLE_RANGE * 2 + 1) * (PCF_SAMPLE_RANGE * 2 + 1);
	}

	return shadow;
}

float3 CalculateDirectionalLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, DirectionalLight dirLight)
{
	float3 fragToLight = normalize(-dirLight.Direction);
	float NoL = clamp(dot(fragNormal, fragToLight), 0.0f, 1.0f);
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	// Check if current fragment is in shadow
	float4 fragPosLS = mul(dirLight.ViewProjection, fragPosWS);
	float shadow = CalculateDirectionalShadow(fragPosLS, NoL, dirLight.ShadowMapIndex);

	// Evaluate BRDF
	float3 halfVec = normalize(viewDir + fragToLight);
	BRDFResult BRDF = CalculateBRDF(viewDir, fragToLight, NoL, albedo, fragNormal, metalness, roughness);
	float3 radiance = dirLight.Color;

	float3 ambient = BRDF.Diffuse * dirLight.Ambient;
	float3 totalBRDF = BRDF.Diffuse + BRDF.Specular;
	Lo = (1.0f - shadow) * totalBRDF * radiance * NoL;

	return ambient + Lo;
}

float3 CalculateSpotLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, SpotLight spotLight)
{
	float distance = length(spotLight.Position - fragPosWS);
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	if (distance <= spotLight.Range)
	{
		float3 fragToLight = normalize(spotLight.Position - fragPosWS.xyz);
		float NoL = clamp(dot(fragNormal, fragToLight), 0.0f, 1.0f);

		if (NoL > 0.0f)
		{
			float angleToLight = dot(fragToLight, -spotLight.Direction);

			if (angleToLight > spotLight.OuterConeAngle)
			{
				// Check if current fragment is in shadow
				float4 fragPosLS = mul(spotLight.ViewProjection, fragPosWS);
				float shadow = CalculateDirectionalShadow(fragPosLS, NoL, spotLight.ShadowMapIndex);

				// Evaluate BRDF
				float3 halfVec = normalize(viewDir + fragToLight);
				BRDFResult BRDF = CalculateBRDF(viewDir, fragToLight, NoL, albedo, fragNormal, metalness, roughness);
				float3 totalBRDF = BRDF.Diffuse + BRDF.Specular;

				// Calculate distance attenuation
				float attenuation = DistanceAttenuation(spotLight.Attenuation, distance);
				float3 radiance = spotLight.Color * attenuation;

				// Calculate cone attenuation
				float epsilon = abs(spotLight.InnerConeAngle - spotLight.OuterConeAngle);
				float coneAttenuation = clamp((angleToLight - spotLight.OuterConeAngle) / epsilon, 0.0f, 1.0f);

				Lo = (1.0f - shadow) * totalBRDF * radiance * NoL;
				Lo *= coneAttenuation;
			}
		}
	}

	return Lo;
}

float3 CalculatePointLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, PointLight pointLight)
{
	float distance = length(pointLight.Position - fragPosWS);
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	if (distance <= pointLight.Range)
	{
		float3 fragToLight = normalize(pointLight.Position - fragPosWS.xyz);
		float NoL = clamp(dot(fragNormal, fragToLight), 0.0f, 1.0f);

		if (NoL > 0.0f)
		{
			// Check if current fragment is in shadow
			float3 lightToFrag = fragPosWS.xyz - pointLight.Position;
			float shadow = CalculateOmnidirectionalShadow(lightToFrag, NoL, pointLight.Range, pointLight.ShadowMapIndex);

			// Evaluate BRDF
			float3 halfVec = normalize(viewDir + fragToLight);
			BRDFResult BRDF = CalculateBRDF(viewDir, fragToLight, NoL, albedo, fragNormal, metalness, roughness);
			float3 totalBRDF = BRDF.Diffuse + BRDF.Specular;

			// Calculate distance attenuation
			float attenuation = DistanceAttenuation(pointLight.Attenuation, distance);
			float3 radiance = pointLight.Color * attenuation;

			Lo = (1.0f - shadow) * totalBRDF * radiance * NoL;
		}
	}

	return Lo;
}
