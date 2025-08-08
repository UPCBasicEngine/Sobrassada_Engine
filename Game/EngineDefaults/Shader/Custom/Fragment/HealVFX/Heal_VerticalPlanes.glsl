#version 460

#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

layout(location=4) uniform bool isWireframe;
layout(location=5) uniform bool isAlphaDiscard;
layout(location=6) uniform vec3 cameraPos;
layout(location=7) uniform float frameTimer;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

out vec4 fragColor;

struct DirectionalLight
{
    vec3 direction;
    vec4 color;             // rbg = color & alpha = intensity
};

struct PointLight
{
	vec4 position;		// xyz = position & w = range
	vec4 color;			// rgb = color & alpha = intensity 
};

struct SpotLight
{
	vec4 position;		// xyz = position & w = range
	vec4 color;			// rgb = color & alpha = intensity
	vec3 direction;	
	float innerAngle;
	float outerAngle;
};

// UBOs
layout(std140, binding = 6) uniform Material
{
    vec4 diffColor;
    vec3 specColor;
    float shininess;
    bool shininessInAlpha;
    float metallicFactor;
    float roughnessFactor;
    uvec2 diffuseTex;
    uvec2 specularTex;
    uvec2 metallicTex;
    uvec2 normalTex;
    int hasSpecular;
    int hasMetallic;
    uvec2 emmisiveTex;
    uvec2 occlusionTex;
    uvec2 padding;
};

layout(std140, binding = 2) uniform Ambient
{
	vec4 ambient_color;		// rbg = color & alpha = intensity
    samplerCube cubemapIrradiance;
    samplerCube cubemapPrefiltered;
    uvec2 environmentBRDF;
    int numLevels;
};

layout(std140, binding = 3) uniform Directional
{
    vec4 directional_dir;
    vec4 directional_color;
};

// SSBOs
readonly layout(std430, binding = 4) buffer PointLights
{
	int pointLightsCount;
	PointLight pointLights[];
};

readonly layout(std430, binding = 5) buffer SpotLights
{
	int spotLightsCount;
	SpotLight spotLights[];
};

float PointLightAttenuation(const int index) 
{
	const float distance = length(pos - pointLights[index].position.xyz);
	return pow(max(1 - pow((distance / pointLights[index].position.w), 4.0), 0.0), 2.0) / (pow(distance, 2.0) + 1.0);
}

float SpotLightAttenuation(const int index)
{
	const vec3 dirLight = normalize(spotLights[index].direction);
	const float distance = dot(pos - spotLights[index].position.xyz, dirLight);
	const float Fatt = pow(max(1 - pow((distance / spotLights[index].position.w), 4.0), 0.0), 2.0) / (pow(distance, 2.0) + 1.0);

	const vec3 D = normalize(pos - spotLights[index].position.xyz);
	const float C = dot(D, dirLight);

	const float cosInner = cos(radians(spotLights[index].innerAngle));
	const float cosOuter = cos(radians(spotLights[index].outerAngle));
	float Catt = 0;
	if (C > cosInner) Catt = 1;
	else if (C < cosInner && C > cosOuter) Catt = (C - cosOuter) / (cosInner - cosOuter);

	return Fatt * Catt;
}

vec3 GetAmbientLight(const in vec3 normal, const in vec3 R, const float NdotV, const float roughness, const in vec3 diffuseColor, const in vec3 specularColor)
{
    const vec3 irradiance = texture(cubemapIrradiance, normal).rgb;

    const vec3 radiance = textureLod(cubemapPrefiltered, R, roughness * (numLevels - 1)).rgb;
    const vec2 fab = texture(sampler2D(environmentBRDF), vec2(NdotV, roughness)).rg;

    const vec3 diffuse = (diffuseColor * (1 - specularColor));

    return diffuse * irradiance + radiance * (specularColor * fab.x + fab.y);
}

float VisibilityFunction(const float NdotL, const float NdotV, const float roughness){
    const float G1 = NdotL * (NdotV * (1 - roughness) + roughness);
    const float G2 = NdotV * (NdotL * (1 - roughness) + roughness);
    return 0.5/(G1 + G2);
}

float GGXNormalDistribution(const float NdotH, const float roughness){
    const float roughness2 = roughness * roughness;
    const float NdotH2 = NdotH * NdotH;
    const float denomTerm = NdotH2 * (roughness2 - 1) + 1;
    const float denominator  = PI * denomTerm * denomTerm;
    return roughness2/denominator;
}

vec3 RenderLight(const vec3 L, const vec3 N, const vec3 Cd, const vec3 Li, const float NdotL, const float roughness, const vec3 RF0)
 {
    const vec3 V = normalize(cameraPos - pos);
    const vec3 H = normalize(V + L);

    const float NdotV = max(dot(N, V), 0.0001);
    const float NdotH = max(dot(N, H), 0.0001);
    
    const float cosTheta = max(dot(L, H), 0.0001);
    const vec3 fresnel = RF0 + (1 - RF0) * pow(1 - cosTheta, 5);

    const float visibility = VisibilityFunction(NdotL, NdotV, roughness);
    const float GGX = GGXNormalDistribution(NdotH, roughness);

    const vec3 diffspec = (Cd * (1-RF0) + 0.25 * fresnel * visibility * GGX) * Li * NdotL;

    return diffspec;
}

vec3 RenderPointLight(const int index, const vec3 N, const vec3 Cd, float roughness, vec3 RF0)
{
	const float attenuation = PointLightAttenuation(index);
	const vec3 L = -normalize(pos - pointLights[index].position.xyz);
	const vec3 Li = pointLights[index].color.rgb * pointLights[index].color.a * attenuation;
	const float NdotL = dot(N, L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(L, N, Cd, Li, NdotL, roughness, RF0);
	else return vec3(0);	
}

vec3 RenderSpotLight(const int index, const vec3 N, const vec3 Cd, float roughness, vec3 RF0)
{
	const float attenuation = SpotLightAttenuation(index);
	const vec3 L = -normalize(pos - spotLights[index].position.xyz);
	const vec3 Li = spotLights[index].color.rgb * spotLights[index].color.a * attenuation;
	float NdotL = dot(N, L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(L, N, Cd, Li, NdotL, roughness, RF0);
	else return vec3(0);
}

mat3 CreateTBN()
{
    const vec3 T = normalize(vec3(tangent));
    const vec3 N = normalize(normal);
    const vec3 B = tangent.w * cross(N, T);
    return mat3(T, B, N);
}

void main()
{
    const vec4 texColor = pow(texture(sampler2D(diffuseTex), uv), vec4(2.2f));
    vec4 metallicRoughnessTexColor;
    if(hasMetallic == 1) metallicRoughnessTexColor = pow(texture(sampler2D(metallicTex), uv), vec4(2.2));
    else metallicRoughnessTexColor = vec4(1);

    // Blender nodes recreation
    vec2 flippedUvs = vec2(uv.x, 1.0f - uv.y);
    float subtractNode = flippedUvs.x - 0.5f;
    float absoluteNode = abs(subtractNode);
    float subtractNode2 = clamp(0.5f - absoluteNode, 0.0f, 1.0f);
    float subtractNode3 = clamp(1.0f - flippedUvs.y, 0.0f, 1.0f);
    float multiplyNode = subtractNode2 * subtractNode3;
    
    float subtractNode4 = 25.0f - frameTimer;
    float multiplyNode2 = subtractNode4 * 0.03f;
    float bol = frameTimer > 14.0f ? 1.0f : 0.0f;
    float multiplyNode3 = multiplyNode2 * bol;
    
    float multiplyNode4 = clamp(multiplyNode * multiplyNode3, 0.0f, 1.0f);
    const float alpha = multiplyNode4 * diffColor.a;

    if (!isWireframe && isAlphaDiscard)
    {
        if(alpha < 0.05) discard;
    }

    vec3 N = normalize(normal);
    // Retrive normal for normal map
    if (normalTex.r != 0 || normalTex.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (texture(sampler2D(normalTex), uv).xyz*2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }

    const float roughness = roughnessFactor * metallicRoughnessTexColor.y;
    //roughness = roughness * roughness;
    const float metallic = metallicFactor * metallicRoughnessTexColor.z;

    const vec3 V = normalize(cameraPos - pos);
    const vec3 R = reflect(-V, N);
    const float NdotV = max(dot(N, V), 0.0001);

    // Ambient light
    const vec3 BaseColor = diffColor.rgb;
    const vec3 Cd = BaseColor * (1 - metallic);
    const vec3 RF0 = mix(vec3(0.04), BaseColor, metallic);

    //vec3 ambient = ambient_color.rgb * ambient_color.a;
    const vec3 ambient = GetAmbientLight(N, R, NdotV, roughness, Cd, RF0);
    vec3 hdr = ambient;

    // Point Lights
    for (int i = 0; i < pointLightsCount; ++i)
	{
		hdr += RenderPointLight(i, N, Cd, roughness, RF0);
	}

    //Spot Lights
    for (int i = 0; i < spotLightsCount; ++i)
	{
		hdr += RenderSpotLight(i, N, Cd, roughness, RF0);
	}

    // Directional light
    const vec3 lightColor = directional_color.rgb * directional_color.a;
    const vec3 L = -normalize(directional_dir.xyz);
    const float NdotL = max(dot(N, L), 0.001f);
    if (NdotL > 0)
    {
		hdr += RenderLight(L, N, Cd, lightColor, NdotL, roughness, RF0);
    }

    const vec4 emissive = vec4(pow(texture(sampler2D(emmisiveTex), uv), vec4(2.2f)));

    hdr += emissive.rgb;

    vec3 ldr = hdr.rgb / (hdr.rgb + vec3(1.0));
    ldr = pow(hdr, vec3(1.0/2.2));
    if (isWireframe)
    {
        fragColor = vec4(ldr, 1.0);
    }
    else {
        fragColor = vec4(ldr, alpha);
    } 
}