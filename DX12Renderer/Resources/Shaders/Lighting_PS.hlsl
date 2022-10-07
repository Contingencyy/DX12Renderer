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
SamplerState Samp2DClamp : register(s0, space1);

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

float CalculateShadow(float4 fragPosLightSpace, uint shadowMapIndex, float angle)
{
	float3 projectedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projectedCoords = projectedCoords * 0.5f + 0.5f;

	float closestDepth = Texture2DTable[shadowMapIndex].Sample(Samp2DClamp, projectedCoords.xy);
	float currentDepth = projectedCoords.z;

	//float bias = max(0.05f * (1.0f - angle), 0.005f);
	//float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	float shadow = currentDepth > closestDepth ? 1.0f : 0.0f;
	return shadow;
}

float3 CalculateDirectionalLight(float4 fragPos, float3 fragNormal, float3 diffuseColor, DirectionalLight dirLight)
{
	float3 ldirection = normalize(-dirLight.Direction);
	float diff = max(dot(fragNormal, ldirection), 0.0f);

	float4 fragPosLightSpace = mul(fragPos, transpose(dirLight.ViewProjection));
	float shadow = CalculateShadow(fragPosLightSpace, dirLight.ShadowMapIndex, diff); // either 1.0 or 0.0

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
			diffuse = spotLight.Diffuse * diff * diffuseColor;
			//float3 specular = pointlight.Specular.xyz * spec * specularColor;

			float epsilon = abs(spotLight.InnerConeAngle - spotLight.OuterConeAngle);
			float coneAttenuation = clamp((angleToLight - spotLight.OuterConeAngle) / epsilon, 0.0f, 1.0f);

			diffuse *= coneAttenuation * attenuation;
			//specular *= coneAttenuation * attenuation;
		}
	}

	return (ambient + diffuse /*+ specular*/);
}
