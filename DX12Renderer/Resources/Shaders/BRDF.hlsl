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
