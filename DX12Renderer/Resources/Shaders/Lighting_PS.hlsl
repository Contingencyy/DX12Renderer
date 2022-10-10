struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float4 WorldPosition : WORLD_POSITION;
	uint2 TexIndices : TEX_INDICES;
};

struct SceneData
{
	matrix ViewProjection;
	float3 Ambient;
	uint NumDirectionalLights;
	uint NumPointLights;
	uint NumSpotLights;
};

struct DirectionalLight
{
	float3 Direction;
	float3 Ambient;
	float3 Diffuse;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct DirectionalLightBuffer
{
	DirectionalLight DirLights[5];
};

struct PointLight
{
	float3 Position;
	float Range;
	float3 Attenuation;
	float3 Ambient;
	float3 Diffuse;

	matrix ViewProjection;
	uint ShadowMapIndex;
};

struct PointLightBuffer
{
	PointLight PointLights[50];
};

struct SpotLight
{
	float3 Position;
	float3 Direction;
	float Range;
	float3 Attenuation;
	float InnerConeAngle;
	float OuterConeAngle;
	float3 Ambient;
	float3 Diffuse;

	matrix ViewProjection;
	uint ShadowMapIndex;
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
SamplerState Samp2DWrap : register(s0, space0);
SamplerState Samp2DBorder : register(s0, space1);

float3 CalculateDirectionalLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, DirectionalLight dirLight);
float3 CalculatePointLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, PointLight pointLight);
float3 CalculateSpotLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, SpotLight spotLight);

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float4 diffuseColor = IN.Color * Texture2DTable[IN.TexIndices.x].Sample(Samp2DWrap, IN.TexCoord);
	float4 textureNormal = Texture2DTable[IN.TexIndices.y].Sample(Samp2DWrap, IN.TexCoord);
	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	finalColor += diffuseColor.xyz * SceneDataCB.Ambient;

	float4 fragPos = IN.WorldPosition;
	float3 fragNormal = normalize(IN.Normal * textureNormal.xyz);

	for (uint d = 0; d < SceneDataCB.NumDirectionalLights; ++d)
	{
		finalColor += CalculateDirectionalLight(fragPos, fragNormal, diffuseColor.xyz, DirLightCB.DirLights[d]);
	}

	for (uint p = 0; p < SceneDataCB.NumPointLights; ++p)
	{
		finalColor += CalculatePointLight(fragPos, fragNormal, diffuseColor.xyz, PointLightCB.PointLights[p]);
	}

	for (uint s = 0; s < SceneDataCB.NumSpotLights; ++s)
	{
		finalColor += CalculateSpotLight(fragPos, fragNormal, diffuseColor.xyz, SpotLightCB.SpotLights[s]);
	}

	return float4(finalColor, diffuseColor.w);
}

static float2 poissonDisk[16] = {
	float2(-0.94201624, -0.39906216),
	float2(0.94558609, -0.76890725),
	float2(-0.094184101, -0.92938870),
	float2(0.34495938, 0.29387760),
	float2(-0.91588581, 0.45771432),
	float2(-0.81544232, -0.87912464),
	float2(-0.38277543, 0.27676845),
	float2(0.97484398, 0.75648379),
	float2(0.44323325, -0.97511554),
	float2(0.53742981, -0.47373420),
	float2(-0.26496911, -0.41893023),
	float2(0.79197514, 0.19090188),
	float2(-0.24188840, 0.99706507),
	float2(-0.81409955, 0.91437590),
	float2(0.19984126, 0.78641367),
	float2(0.14383161, -0.14100790)
};

float GetRandom(float3 seed, int i)
{
	float4 seed4 = (seed, i);
	float dotprod = dot(seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
	return frac(sin(dotprod) * 43758.5453f);
}

float CalculateShadow(float4 fragPosLightSpace, uint shadowMapIndex, float angle)
{
	float3 projectedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	float currentDepth = projectedCoords.z;
	projectedCoords = projectedCoords * 0.5f;
	projectedCoords.xy += 0.5f;

	// Invert Y for D3D style screen coordinates
	projectedCoords.y = 1.0f - projectedCoords.y;

	float bias = max(0.000008f * (1.0f - angle), 0.000004f);
	float shadow = 0.0f;

	// One sample
	/*float closestDepth = Texture2DTable[shadowMapIndex].Sample(Samp2DBorder, projectedCoords.xy);
	shadow = currentDepth + bias < closestDepth ? 1.0f : 0.0f;*/

	// Apply percentage closer filtering with poisson sampling
	float2 texelSize = 0.25f / float2(1024.0f, 1024.0f);
	float diskDenom = 1.0f / 2048.0f;
	int numDiskSamples = 4;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			for (int i = 0; i < numDiskSamples; ++i)
			{
				int poissonIndex = 16.0f * GetRandom(fragPosLightSpace.xyz, i) % 16.0f;
				float2 pcfDepth = Texture2DTable[shadowMapIndex].Sample(Samp2DBorder, projectedCoords.xy + (poissonDisk[poissonIndex] * diskDenom) + (float2(x, y) * texelSize)).r;
				shadow += currentDepth + bias < pcfDepth ? 1.0f : 0.0f;
			}
		}
	}
	shadow /= 9 * numDiskSamples;

	return shadow;
}

float3 CalculateDirectionalLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, DirectionalLight dirLight)
{
	float3 ldirection = normalize(-dirLight.Direction);
	float diff = max(dot(fragNormal, ldirection), 0.0f);

	float4 fragPosLightSpace = mul(dirLight.ViewProjection, fragPos);
	float shadow = CalculateShadow(fragPosLightSpace, dirLight.ShadowMapIndex, dot(fragNormal, ldirection)); // either 1.0 or 0.0

	//float3 reflectDirection = reflect(-ldirection, fragNormal);
	//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

	float3 ambient = dirLight.Ambient * diffuseColor;
	float3 diffuse = dirLight.Diffuse * diff * diffuseColor;
	//float3 specular = pointlight.Specular.xyz * spec * specularColor;

	return (ambient + (1.0f - shadow) * (diffuse /*+ specular*/));
}

float3 CalculatePointLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, PointLight pointLight)
{
	float3 ldirection = pointLight.Position - fragPos.xyz;
	float distance = length(ldirection);

	float3 ambient = float3(0.0f, 0.0f, 0.0f);
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	//float3 specular = float3(0.0f, 0.0f, 0.0f);

	if (distance <= pointLight.Range)
	{
		ldirection /= distance;

		// After calculating the distance attenuation, we need to rescale the value to account for the LIGHT_RANGE_EPSILON at which the light is cutoff/ignored
		float attenuation = clamp(1.0f / (pointLight.Attenuation.x + (pointLight.Attenuation.y * distance) + (pointLight.Attenuation.z * (distance * distance))), 0.0f, 1.0f);
		attenuation = (attenuation - 0.01f) / (1.0f - 0.01f);
		attenuation = max(attenuation, 0.0f);

		ambient = pointLight.Ambient * diffuseColor;
		ambient *= attenuation;

		//float3 reflectDirection = reflect(-ldirection, fragNormal);
		//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

		float diff = max(dot(fragNormal, ldirection), 0.0f);
		diffuse = pointLight.Diffuse * diff * diffuseColor;
		//float3 specular = pointlight.Specular.xyz * spec * specularColor;

		diffuse *= attenuation;
		//specular *= attenuation;
	}

	/*if (distance > pointLight.Range)
		return float3(1.0f, 0.0f, 1.0f);*/

	return (ambient + diffuse /*+ specular*/);
}

float3 CalculateSpotLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, SpotLight spotLight)
{
	float3 ldirection = spotLight.Position - fragPos.xyz;
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
			//float3 reflectDirection = reflect(-ldirection, fragNormal);
			//float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

			float diff = max(dot(fragNormal, ldirection), 0.0f);

			float4 fragPosLightSpace = mul(spotLight.ViewProjection, fragPos);
			shadow = CalculateShadow(fragPosLightSpace, spotLight.ShadowMapIndex, dot(fragNormal, ldirection)); // either 1.0 or 0.0

			diffuse = spotLight.Diffuse * diff * diffuseColor;
			//float3 specular = pointlight.Specular.xyz * spec * specularColor;

			float epsilon = abs(spotLight.InnerConeAngle - spotLight.OuterConeAngle);
			float coneAttenuation = clamp((angleToLight - spotLight.OuterConeAngle) / epsilon, 0.0f, 1.0f);

			diffuse *= coneAttenuation * attenuation;
			//specular *= coneAttenuation * attenuation;
		}
	}

	return (ambient + (1.0f - shadow) * diffuse /*+ specular*/);
}
