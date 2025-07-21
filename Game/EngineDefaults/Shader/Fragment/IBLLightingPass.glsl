#version 460

#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

layout(binding = 0) uniform sampler2D gDiffuse;
layout(binding = 1) uniform sampler2D gSpecular;
layout(binding = 2) uniform sampler2D gPosition;
layout(binding = 3) uniform sampler2D gNormal;
layout(binding = 4) uniform sampler2D shadowMap;
uniform mat4 viewLight;
uniform mat4 projLight;
uniform vec3 shadowTint;

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024

uniform vec2 screenSize;
uniform int numTilesX;

in vec2 uv0;

out vec4 outColor;

uniform vec3 cameraPos;

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

layout(std430, binding = 6) readonly buffer VisibleLightIndicesBuffer {
    int visibleIndices[];
};

// UBOs
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
//We don't need pointLightsCount and spotLightsCount but we left here as is simpler for the rest of the code
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


float PointLightAttenuation(const int index, vec3 pos) 
{
	const float distance = length(pos - pointLights[index].position.xyz);
	return pow(max(1 - pow((distance / pointLights[index].position.w), 4.0), 0.0), 2.0) / (pow(distance, 2.0) + 1.0);
}

float SpotLightAttenuation(const int index, vec3 pos)
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

vec3 RenderLight(const vec3 L, const vec3 N, const vec3 Cd, const vec3 Li, const float NdotL, const float roughness, const vec3 RF0, vec3 pos, bool hasShadows)
 {
    const vec3 V = normalize(cameraPos - pos);
    const vec3 H = normalize(V + L);

    const float NdotV = max(dot(N, V), 0.0001);
    const float NdotH = max(dot(N, H), 0.0001);
    
    const float cosTheta = max(dot(L, H), 0.0001);
    const vec3 fresnel = RF0 + (1 - RF0) * pow(1 - cosTheta, 5);

    const float visibility = VisibilityFunction(NdotL, NdotV, roughness);
    const float GGX = GGXNormalDistribution(NdotH, roughness);

    vec3 diffspec = (Cd * (1-RF0) + 0.25 * fresnel * visibility * GGX) * Li * NdotL;

    //Shadows
    if(hasShadows){
        float shadow = 0.0;
        vec4 pos_from_light = projLight * viewLight * vec4(pos, 1.0);
        vec3 projCoords = pos_from_light.xyz / pos_from_light.w;
        projCoords = projCoords * 0.5 + 0.5;

        if(projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
        projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
        projCoords.z >= 0.0 && projCoords.z <= 1.0)
        {
            //PCF
            float factor = 0.0;

            vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
            for (int yOffset = -1; yOffset <= 1; ++yOffset) {
                for (int xOffset = -1; xOffset <= 1; ++xOffset) {
                    vec2 offset = vec2(float(xOffset), float(yOffset)) * texelSize;
                    float sampledDepth = texture(shadowMap, projCoords.xy + offset).r;

                    if (projCoords.z - 0.001 > sampledDepth)
                        factor += 1.0;
                }
            }

            shadow = factor;
        }

        //diffspec = mix(diffspec, diffspec * vec3(0.56, 0.78, 0.90), shadow);
        //diffspec = mix(diffspec, diffspec * vec3(0.0, 0.0, 0.0), shadow);
        diffspec = mix(diffspec, diffspec * shadowTint, shadow);
    }

    return diffspec;
}

vec3 RenderPointLight(const int index, const vec3 N, const vec3 Cd, float roughness, vec3 RF0, vec3 pos)
{
	const float attenuation = PointLightAttenuation(index, pos);
	const vec3 L = -normalize(pos - pointLights[index].position.xyz);
	const vec3 Li = pointLights[index].color.rgb * pointLights[index].color.a * attenuation;
	const float NdotL = dot(N, L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(L, N, Cd, Li, NdotL, roughness, RF0, pos, false);
	else return vec3(0);	
}

vec3 RenderSpotLight(const int index, const vec3 N, const vec3 Cd, float roughness, vec3 RF0, vec3 pos)
{
	const float attenuation = SpotLightAttenuation(index, pos);
	const vec3 L = -normalize(pos - spotLights[index].position.xyz);
	const vec3 Li = spotLights[index].color.rgb * spotLights[index].color.a * attenuation;
	float NdotL = dot(N, L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(L, N, Cd, Li, NdotL, roughness, RF0, pos, false);
	else return vec3(0);
}

void main()
{
    const vec4 texColor = texture(gDiffuse, uv0);
    const vec4 metallicRoughnessTexColor = texture(gSpecular, uv0);
    const float alpha = texColor.a;
    const vec3 pos = texture(gPosition, uv0).rgb;
    const vec3 normal = texture(gNormal, uv0).rgb;

    vec3 N = normalize(normal);

    const float roughness = metallicRoughnessTexColor.y;
    //roughness = roughness * roughness;
    const float metallic = metallicRoughnessTexColor.z;

    const vec3 V = normalize(cameraPos - pos);
    const vec3 R = reflect(-V, N);
    const float NdotV = max(dot(N, V), 0.0001);

    // Ambient light
    const vec3 BaseColor = texColor.rgb;
    const vec3 Cd = BaseColor * (1 - metallic);
    const vec3 RF0 = mix(vec3(0.04), BaseColor, metallic);

    //vec3 ambient = ambient_color.rgb * ambient_color.a;
    const vec3 ambient = GetAmbientLight(N, R, NdotV, roughness, Cd, RF0);
    vec3 hdr = ambient;

    ivec2 tileCoord = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    int tileIndex = tileCoord.y * numTilesX + tileCoord.x;
    int baseOffset = tileIndex * MAX_LIGHTS_PER_TILE;

    for (int i = 0; i < MAX_LIGHTS_PER_TILE; ++i)
    {
        int lightIndex = visibleIndices[baseOffset + i];
        if (lightIndex == -1) break;

        bool isSpot = (lightIndex & 0x80000000) != 0;
        int realIndex = lightIndex & 0x7FFFFFFF;

        if (isSpot) {
            hdr += RenderSpotLight(realIndex, N, Cd, roughness, RF0, pos);
        } else {
            hdr += RenderPointLight(realIndex, N, Cd, roughness, RF0, pos);
        }
    }

    // Directional light
    const vec3 lightColor = directional_color.rgb * directional_color.a;
    const vec3 L = -normalize(directional_dir.xyz);
    const float NdotL = max(dot(N, L), 0.001f);
    if (NdotL > 0)
    {
		hdr += RenderLight(L, N, Cd, lightColor, NdotL, roughness, RF0, pos, true);
    }

    vec3 ldr = hdr.rgb / (hdr.rgb + vec3(1.0));
    ldr = pow(hdr, vec3(1.0/2.2));
    outColor = vec4(ldr, alpha);
}