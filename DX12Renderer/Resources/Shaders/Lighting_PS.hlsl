#include "Common.hlsl"
#include "BRDF.hlsl"

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float4 CurrentPosNoJitter : CURRENT_POS_NO_JITTER;
	float4 PreviousPosNoJitter : PREVIOUS_POS_NO_JITTER;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float4 WorldPosition : WORLD_POSITION;
	uint MaterialID : MATERIAL_ID;
};

ConstantBuffer<GlobalConstantBufferData> GlobalCB : register(b0);
ConstantBuffer<SceneData> SceneDataCB : register(b1);
ConstantBuffer<MaterialCBData> MaterialCB : register(b2);
ConstantBuffer<LightCBData> LightCB : register(b3);

Texture2D Texture2DTable[] : register(t0, space0);
TextureCube TextureCubeTable[] : register(t0, space1);
SamplerState Sampler_Antisotropic_Wrap : register(s0, space0);
SamplerComparisonState Sampler_PCF : register(s0, space1);

float3 EvaluateDirectionalLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, DirectionalLight dirLight);
float3 EvaluateSpotLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, SpotLight spotLight);
float3 EvaluatePointLight(float4 fragPosWS, float3 fragNormal, float3 albedo, float metalness, float roughness, float3 viewDir, PointLight pointLight);

struct PixelShaderOutput
{
	float4 HDR : SV_TARGET0;
	float2 Velocity : SV_TARGET1;
};

PixelShaderOutput main(PixelShaderInput IN)
{
	Material mat = MaterialCB.materials[IN.MaterialID];

	// Sample albedo color and normals from normal map
	float4 albedo = Texture2DTable[mat.AlbedoTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float3 normalTS = Texture2DTable[mat.NormalTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord).xyz;
	normalTS = (normalTS * 2.0f) - 1.0f;

	// Sample metallic roughness and scale them by the factor in the material
	float4 metallicRoughness = Texture2DTable[mat.MetallicRoughnessTextureIndex].Sample(Sampler_Antisotropic_Wrap, IN.TexCoord);
	float metalness = metallicRoughness.b * mat.Metalness;
	float roughness = metallicRoughness.g * mat.Roughness;

	// Get the fragments world position, the fragments world space normal and the view direction
	float4 fragPosWS = IN.WorldPosition;
	float3 fragNormalWS = normalize(normalTS.x * IN.Tangent + normalTS.y * IN.Bitangent + normalTS.z * IN.Normal);
	float3 viewDir = normalize(SceneDataCB.ViewPosition - fragPosWS);

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	// Evalulate color from all light sources
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

	// Render HDR color to SV_Target0
	PixelShaderOutput OUT;
	OUT.HDR = float4(finalColor, albedo.w);

	// Calculate velocity and render to SV_Target1
	float2 currentUV = (IN.CurrentPosNoJitter.xyz / IN.CurrentPosNoJitter.w) * float2(0.5f, 0.5f) + float2(0.5f, 0.5f);
	currentUV.y *= -1.0f;
	float2 previousUV = (IN.PreviousPosNoJitter.xyz / IN.PreviousPosNoJitter.w) * float2(0.5f, 0.5f) + float2(0.5f, 0.5f);
	previousUV.y *= -1.0f;
	OUT.Velocity = currentUV.xy - previousUV.xy;

	fragNormalWS = 0.5 * fragNormalWS + 0.5;
	OUT.HDR = float4(fragNormalWS, 1.0);

	return OUT;
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

float SamplePCF(float2 baseUV, float uOff, float vOff, float compDepth, float invShadowMapRes, uint shadowMapIndex)
{
	float2 uv = baseUV + float2(uOff, vOff) * invShadowMapRes;
	return Texture2DTable[shadowMapIndex].SampleCmpLevelZero(Sampler_PCF, uv, compDepth);
}

float SampleShadowPCF(float2 uv, float compDepth, uint shadowMapIndex)
{
	float2 shadowMapRes;
	Texture2DTable[shadowMapIndex].GetDimensions(shadowMapRes.x, shadowMapRes.y);
	float2 invShadowMapRes = 1.0f / shadowMapRes;

	uv *= shadowMapRes;

	float2 baseUV;
	baseUV.x = floor(uv.x + 0.5f);
	baseUV.y = floor(uv.y + 0.5f);

	float s = (uv.x + 0.5f - baseUV.x);
	float t = (uv.y + 0.5f - baseUV.y);

	baseUV -= float2(0.5f, 0.5f);
	baseUV *= invShadowMapRes;

	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;

	float acc = 0.0f;

	acc += uw0 * vw0 * SamplePCF(baseUV, u0, v0, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw0 * SamplePCF(baseUV, u1, v0, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw0 * SamplePCF(baseUV, u2, v0, compDepth, invShadowMapRes, shadowMapIndex);

	acc += uw0 * vw1 * SamplePCF(baseUV, u0, v1, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw1 * SamplePCF(baseUV, u1, v1, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw1 * SamplePCF(baseUV, u2, v1, compDepth, invShadowMapRes, shadowMapIndex);

	acc += uw0 * vw2 * SamplePCF(baseUV, u0, v2, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw2 * SamplePCF(baseUV, u1, v2, compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw2 * SamplePCF(baseUV, u2, v2, compDepth, invShadowMapRes, shadowMapIndex);

	return acc *= 1.0f / 144;
}

float SamplePCFCube(float3 lightToFrag, float3 baseDir, float3 dirOffset, float compDepth, float invShadowMapRes, uint shadowMapIndex)
{
	float3 offset = baseDir + dirOffset * invShadowMapRes;
	float3 normDir = normalize(lightToFrag);
	return TextureCubeTable[shadowMapIndex].SampleCmpLevelZero(Sampler_PCF, float3(normDir.x + offset.x, normDir.y + offset.y, normDir.z + offset.z), compDepth);
}

float SampleShadowPCF(float3 lightToFrag, float compDepth, uint shadowMapIndex)
{
	float2 shadowMapRes;
	Texture2DTable[shadowMapIndex].GetDimensions(shadowMapRes.x, shadowMapRes.y);
	float2 invShadowMapRes = 1.0f / shadowMapRes;

	float3 nDir = normalize(lightToFrag) * float3(shadowMapRes, 1.0f);
	float3 baseDir;
	baseDir.x = (nDir.x + 0.5f);
	baseDir.y = (nDir.y + 0.5f);
	baseDir.z = nDir.z;

	float s = (nDir.x + 0.5f - baseDir.x);
	float t = (nDir.y + 0.5f - baseDir.y);

	baseDir.xy -= float2(0.5f, 0.5f);
	baseDir.xy *= invShadowMapRes.xy;

	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;

	float acc = 0.0f;

	acc += uw0 * vw0 * SamplePCFCube(lightToFrag, baseDir, float3(u0, v0, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw0 * SamplePCFCube(lightToFrag, baseDir, float3(u1, v0, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw0 * SamplePCFCube(lightToFrag, baseDir, float3(u2, v0, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);

	acc += uw0 * vw1 * SamplePCFCube(lightToFrag, baseDir, float3(u0, v1, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw1 * SamplePCFCube(lightToFrag, baseDir, float3(u1, v1, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw1 * SamplePCFCube(lightToFrag, baseDir, float3(u2, v1, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);

	acc += uw0 * vw2 * SamplePCFCube(lightToFrag, baseDir, float3(u0, v2, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw1 * vw2 * SamplePCFCube(lightToFrag, baseDir, float3(u1, v2, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);
	acc += uw2 * vw2 * SamplePCFCube(lightToFrag, baseDir, float3(u2, v2, baseDir.z), compDepth, invShadowMapRes, shadowMapIndex);

	return acc *= 1.0f / 144;
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
		shadow = SampleShadowPCF(projectedCoords.xy, currentDepth, shadowMapIndex);

	return shadow;
}

float EvaluateOmnidirectionalShadow(float3 lightToFrag, float angle, float farPlane, uint shadowMapIndex)
{
	float currentDepth = DirectionToDepthValue(lightToFrag, farPlane, 0.1f);
	float shadow = 0.0f;

	if (currentDepth > 1.0f || currentDepth < 0.0f)
		shadow = 1.0f;
	else
		shadow = SampleShadowPCF(lightToFrag, currentDepth, shadowMapIndex);

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
