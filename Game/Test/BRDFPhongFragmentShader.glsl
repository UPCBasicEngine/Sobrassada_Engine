#version 460

in vec3 pos;
in vec2 uv0;
in vec3 normal;
in vec4 tangent;

out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D specularTexture;
layout(binding = 2) uniform sampler2D normal_map;

uniform vec3 diffFactor;
uniform vec3 specFactor;
uniform vec3 cameraPos;
uniform float ambientIntensity;

struct DirectionalLight
{
    vec3 direction;
    vec4 color;
};

layout(std140, binding = 7) uniform Directional
{
    vec4 directional_dir;
    vec4 directional_color;
};

mat3 CreateTBN(const vec3 N, const vec4 T)
{
    vec3 Tn = normalize(vec3(T));
    vec3 B = T.w * normalize(cross(N, Tn));
    return mat3(Tn, B, normalize(N));
}


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

layout(std140, binding = 4) uniform Ambient
{
	vec4 ambient_color;		// rbg = color & alpha = intensity
};

readonly layout(std430, binding = 5) buffer PointLights
{
	int pointLightsCount;
	PointLight pointLights[];
};

readonly layout(std430, binding = 6) buffer SpotLights
{
	int spotLightsCount;
	SpotLight spotLights[];
};

float pointLightAttenuation(const int index) 
{
	float distance = length(pos - pointLights[index].position.xyz);
	return pow(max(1 - pow((distance / pointLights[index].position.w), 4), 0), 2) / (pow(distance, 2) + 1);
}

float spotLightAttenuation(const int index)
{
	vec3 dirLight = normalize(spotLights[index].direction);
	float distance = dot(pos - spotLights[index].position.xyz, dirLight);
	float Fatt = pow(max(1 - pow((distance / spotLights[index].position.w), 4), 0), 2) / (pow(distance, 2) + 1);

	vec3 D = normalize(pos - spotLights[index].position.xyz);
	float C = dot(D, dirLight);

	float cosInner = cos(radians(spotLights[index].innerAngle));
	float cosOuter = cos(radians(spotLights[index].outerAngle));
	float Catt = 0;
	if (C > cosInner) Catt = 1;
	else if (C < cosInner && C > cosOuter) Catt = (C - cosOuter) / (cosInner - cosOuter);

	return Fatt * Catt;
}

vec3 RenderLight(const vec3 N, const vec3 texColor, const float alpha, const vec3 L, const vec3 Li, const float NdotL)
{
   	vec3 specTexColor = texture(specularTexture, uv0).rgb;
    float shininess = alpha * 256.0f;

	float normalization = (shininess + 2.0) / (2.0 * 3.1415926535);
	vec3 V = normalize(cameraPos - pos);
	vec3 R = reflect(L, N);
	float VR = pow(max(dot(V, R), 0.0f), shininess);
	
	shininess = (1 - alpha);
	float RF0 = pow((shininess - 1)/(shininess + 1), 2);
    float cosTheta = max(dot(N, V), 0.0);
    float fresnel = RF0 + (1 - RF0) * pow(1-cosTheta, 5);

	vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * Li * NdotL;
	vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * Li * NdotL;
	return diffuse + specular;
}

vec3 RenderPointLight(const int index, const vec3 N, const vec3 texColor, const float alpha)
{
	float attenuation = pointLightAttenuation(index);
	vec3 L = normalize(pos - pointLights[index].position.xyz);
	vec3 Li = pointLights[index].color.rgb * pointLights[index].color.a * attenuation;
	float NdotL = dot(N, -L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(N, texColor, alpha, L, Li, NdotL);
	else return vec3(0);	
}

vec3 RenderSpotLight(const int index, const vec3 N, const vec3 texColor, const float alpha)
{
	float attenuation = spotLightAttenuation(index);
	vec3 L = normalize(pos - spotLights[index].position.xyz);
	vec3 Li = spotLights[index].color.rgb * spotLights[index].color.a * attenuation;
	float NdotL = dot(N, -L);

	if (NdotL > 0 && attenuation > 0) return RenderLight(N, texColor, alpha, L, Li, NdotL);
	else return vec3(0);
}

void main()
{
    // Retrive normal for normal map
    vec3 texNormal = texture(normal_map, uv0).xyz;
    texNormal = normalize(texNormal * 2.0 - 1.0);

    // Transform normal to world space
    mat3 tbn = CreateTBN(normal, tangent);
    vec3 finalNormal = normalize(tbn * texNormal);

    vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
    float alpha = texture(specularTexture, uv0).a;

    // Retrive directional light
    vec3 lightDir = normalize(-directional_dir.xyz); // Direction where the light comes
    vec3 lightColor = directional_color.rgb;
    float lightIntensity = directional_color.a;


    vec3 N = finalNormal;
    float NL = dot(N, lightDir);

	vec3 ambient = ambient_color.rgb * ambient_color.a;
	vec3 hdr = ambient * texColor;

	for (int i = 0; i < pointLightsCount; ++i)
	{
		hdr += RenderPointLight(i, N, texColor, alpha);
	}

	for (int i = 0; i < spotLightsCount; ++i)
	{
		hdr += RenderSpotLight(i, N, texColor, alpha);
	}

	hdr += RenderLight(N, texColor, alpha, lightDir, lightColor*lightIntensity, dot(N, -lightDir));

	// Tone mapping
	vec3 ldr = hdr.rgb / (hdr.rgb + vec3(1.0));
	ldr = pow(ldr, vec3(1/2.2));
	
	outColor =  vec4(ldr, alpha);
}
