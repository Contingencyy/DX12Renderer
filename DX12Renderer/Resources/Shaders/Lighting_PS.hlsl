#include "DataStructs.hlsl"

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float4 WorldPosition : WORLD_POSITION;
	uint MaterialID : MATERIAL_ID;
};

ConstantBuffer<SceneData> SceneDataCB : register(b0);
ConstantBuffer<MaterialCBData> MaterialCB : register(b1);
ConstantBuffer<LightCBData> LightCB : register(b2);

Texture2D Texture2DTable[] : register(t0, space0);
TextureCube TextureCubeTable[] : register(t0, space1);
SamplerState Sampler_Antisotropic_Wrap : register(s0, space0);
SamplerState Sampler_Linear_Border : register(s0, space1);

float3 EvaluateDirectionalLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, DirectionalLight dirLight);
float3 EvaluateSpotLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, SpotLight spotLight);
float3 EvaluatePointLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, PointLight pointLight);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	Material mat = MaterialCB.materials[IN.MaterialID];

	float4 albedo = Texture2DTable[mat.AlbedoTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float3 normalTS = Texture2DTable[mat.NormalTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord).xyz;
	normalTS = (normalTS * 2.0f) - 1.0f;

	float4 metallicRoughness = Texture2DTable[mat.MetallicRoughnessTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float metalness = metallicRoughness.b * mat.Metalness;
	float roughness = metallicRoughness.g * mat.Roughness;

	float4 fragPosWS = IN.WorldPosition;
	float3 fragNormalWS = normalize(normalTS.x * IN.Tangent + normalTS.y * IN.Bitangent + normalTS.z * IN.Normal);
	float3 viewDir = normalize(SceneDataCB.ViewPosition - fragPosWS);

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	if (SceneDataCB.NumDirectionalLights == 1)
		finalColor += EvaluateDirectionalLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.DirLight);

	for (uint s = 0; s < SceneDataCB.NumSpotLights; ++s)
	{
		finalColor += EvaluateSpotLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.SpotLights[s]);
	}

	for (uint p = 0; p < SceneDataCB.NumPointLights; ++p)
	{
		finalColor += EvaluatePointLight(fragPosWS, fragNormalWS, albedo, metalness, roughness, viewDir, LightCB.PointLights[p]);
	}

	return float4(finalColor, albedo.w);
}

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
	return 1.0f * INV_PI;
}

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
//	return lightScatter * viewScatter * (1.0f * INV_PI);
//}

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

	float a = roughness * roughness;

	float D = NormalDist_GGX(NoH, a);
	float3 F = SchlickFresnel(LoH, f0);
	float V = SmithHeightCorrelated_GGX(NoV, NoL, a);

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

float BoxBlur4x4(float4x4 gather, float currentDepth)
{
	const uint blurSize = 4;
	float accumulator = 0.0f;

	for (uint x = 0; x < blurSize; ++x)
	{
		for (uint y = 0; y < blurSize; ++y)
		{
			if (gather[x][y] > currentDepth)
				accumulator += 1.0f;
		}
	}
	
	accumulator /= pow(blurSize, 2);
	return accumulator;
}

float SampleShadowGather4BoxBlur(float2 uv, float currentDepth, uint shadowMapIndex)
{
	Texture2D<float4> shadowMap = Texture2DTable[shadowMapIndex];

	float4 gather0 = shadowMap.Gather(Sampler_Linear_Border, uv, int2(-1, -1));
	float4 gather1 = shadowMap.Gather(Sampler_Linear_Border, uv, int2(1, -1));
	float4 gather2 = shadowMap.Gather(Sampler_Linear_Border, uv, int2(-1, 1));
	float4 gather3 = shadowMap.Gather(Sampler_Linear_Border, uv, int2(1, 1));

	return BoxBlur4x4(float4x4(gather0, gather1, gather2, gather3), currentDepth);
}

float SampleShadowGather4BoxBlur(float3 lightToFrag, float currentDepth, uint shadowMapIndex)
{
	TextureCube<float4> shadowMap = TextureCubeTable[shadowMapIndex];
	float2 textureSize;
	shadowMap.GetDimensions(textureSize.x, textureSize.y);

	float texelSize = 1.0f / textureSize.x;
	float lightDistance = length(lightToFrag);

	float3 uvwOffset0 = float3(-1.0f, -1.0f, 0.0f) * lightDistance * texelSize;
	float3 uvwOffset1 = float3(1.0f, -1.0f, 0.0f) * lightDistance * texelSize;
	float3 uvwOffset2 = float3(-1.0f, 1.0f, 0.0f) * lightDistance * texelSize;
	float3 uvwOffset3 = float3(1.0f, 1.0f, 0.0f) * lightDistance * texelSize;

	float4 gather0 = shadowMap.Gather(Sampler_Linear_Border, (lightToFrag.xyz + uvwOffset0));
	float4 gather1 = shadowMap.Gather(Sampler_Linear_Border, (lightToFrag.xyz + uvwOffset1));
	float4 gather2 = shadowMap.Gather(Sampler_Linear_Border, (lightToFrag.xyz + uvwOffset2));
	float4 gather3 = shadowMap.Gather(Sampler_Linear_Border, (lightToFrag.xyz + uvwOffset3));

	return BoxBlur4x4(float4x4(gather0, gather1, gather2, gather3), currentDepth);
}

float EvaluateDirectionalShadow(float4 fragPosLS, float angle, uint shadowMapIndex)
{
	float3 projectedCoords = fragPosLS.xyz / fragPosLS.w;
	float currentDepth = projectedCoords.z;
	projectedCoords = projectedCoords * 0.5f;
	projectedCoords.xy += 0.5f;
	// Invert Y for D3D style screen coordinates
	projectedCoords.y = 1.0f - projectedCoords.y;

	float shadow = 0.0f;

	if (projectedCoords.z > 1.0f || projectedCoords.z < 0.0f)
		shadow = 1.0f;
	else
		shadow = SampleShadowGather4BoxBlur(projectedCoords.xy, currentDepth, shadowMapIndex);

	return shadow;
}

float EvaluateOmnidirectionalShadow(float3 lightToFrag, float angle, float farPlane, uint shadowMapIndex)
{
	float currentDepth = DirectionToDepthValue(lightToFrag, farPlane, 0.1f);
	float shadow = 0.0f;

	if (currentDepth > 1.0f || currentDepth < 0.0f)
		shadow = 1.0f;
	else
		shadow = SampleShadowGather4BoxBlur(lightToFrag, currentDepth, shadowMapIndex);

	return shadow;
}

float3 EvaluateDirectionalLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, DirectionalLight dirLight)
{
	float3 fragToLight = normalize(-dirLight.Direction);
	float NoL = clamp(dot(fragNormal, fragToLight), 0.0f, 1.0f);
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	// Check if current fragment is in shadow
	float4 fragPosLS = mul(dirLight.ViewProjection, fragPosWS);
	float shadow = EvaluateDirectionalShadow(fragPosLS, NoL, dirLight.ShadowMapIndex);

	// Evaluate BRDF
	float3 halfVec = normalize(viewDir + fragToLight);
	BRDFResult BRDF = CalculateBRDF(viewDir, fragToLight, NoL, albedo, fragNormal, metalness, roughness);
	float3 radiance = dirLight.Color;

	float3 ambient = BRDF.Diffuse * dirLight.Ambient;
	float3 totalBRDF = BRDF.Diffuse + BRDF.Specular;
	Lo = (1.0f - shadow) * totalBRDF * radiance * NoL;

	return ambient + Lo;
}

float3 EvaluateSpotLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, SpotLight spotLight)
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
				float shadow = EvaluateDirectionalShadow(fragPosLS, NoL, spotLight.ShadowMapIndex);

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

float3 EvaluatePointLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, PointLight pointLight)
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
			float shadow = EvaluateOmnidirectionalShadow(lightToFrag, NoL, pointLight.Range, pointLight.ShadowMapIndex);

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
