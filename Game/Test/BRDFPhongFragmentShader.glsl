#version 460

in vec3 pos;
in vec2 uv0;
in vec3 normal;
out vec4 outColor;

layout(binding=0) uniform sampler2D diffuseTexture;
layout(binding=1) uniform sampler2D specularTexture;

uniform vec3 diffFactor;
uniform vec3 specFactor;

uniform vec3 lightColor;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float ambientIntensity;

// Testing point light uniforms
layout(location = 3) uniform vec3 pointColor;
layout(location = 4) uniform vec3 pointPosition;
layout(location = 5) uniform float pointIntensity;
layout(location = 6) uniform float pointRange;

// Testing spotlight uniforms
layout(location = 7) uniform vec3 spotColor;
layout(location = 8) uniform vec3 spotPosition;
layout(location = 9) uniform vec3 spotDirection;
layout(location = 10) uniform float spotIntensity;
layout(location = 11) uniform float spotRange;
layout(location = 12) uniform float spotInnerAngle;
layout(location = 13) uniform float spotOuterAngle;


struct PointLight
{
	vec4 position;	//xyz = position & w = radius
	vec4 color;		// rbg = color & alpha = intensity 
};

readonly layout(std430, binding = 5) buffer PointLights
{
	float pointLightsCount;
	PointLight pointLights[];
};

float pointLightAttenuation() 
{
	float distance = length(pos - pointPosition);
	return pow(max(1 - pow((distance / pointRange), 4), 0), 2) / (pow(distance, 2) + 1);
}

float spotLightAttenuation()
{
	vec3 dirLight = normalize(spotDirection);
	float distance = dot(pos - spotPosition, dirLight);
	float Fatt = pow(max(1 - pow((distance / spotRange), 4), 0), 2) / (pow(distance, 2) + 1);

	vec3 D = normalize(pos - spotPosition);
	float C = dot(D, dirLight);

	float cosInner = cos(radians(spotInnerAngle));
	float cosOuter = cos(radians(spotOuterAngle));
	float Catt = 0;
	if (C > cosInner) Catt = 1;
	else if (C < cosInner && C > cosOuter) Catt = (C - cosOuter) / (cosInner - cosOuter);

	return Fatt * Catt;
}

vec3 RenderPointLight(const vec3 N, const vec3 texColor, const float alpha)
{
	vec3 Lpoint = normalize(pos - pointPosition);
	vec3 LiPoint = pointColor * pointLights[0].color.a * pointLightAttenuation();
	float NLpoint = dot(N, -Lpoint);

	if (NLpoint > 0)
	{
   		vec3 specTexColor = texture(specularTexture, uv0).rgb;
        float shininess = alpha * 256.0f;

		float normalization = (shininess + 2.0) / (2.0 * 3.1415926535);
		vec3 V = normalize(cameraPos - pos);
		vec3 R = reflect(Lpoint, N);
		float VR = pow(max(dot(V, R), 0.0f), shininess);
		
		shininess = (1 - alpha);
		float RF0 = pow((shininess - 1)/(shininess + 1), 2);
        float cosTheta = max(dot(N, V), 0.0);
        float fresnel = RF0 + (1 - RF0) * pow(1-cosTheta, 5);

		//vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * lightColor * NL;
		//vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * lightColor * NL;

		vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * LiPoint * NLpoint;
		vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * LiPoint * NLpoint;
		return diffuse + specular;
	}
	else
	{
		return vec3(0);
	}
}

void main()
{
	vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
	float alpha = texture(specularTexture, uv0).a;

	vec3 ambient = ambientIntensity * texColor;
	vec3 hdr = ambient;

	vec3 N = normalize(normal);
   	//vec3 L = normalize(lightDir);
	//float NL = dot(N, -L);

	//for ()
	hdr += RenderPointLight(N, texColor, alpha);

	vec3 Lspot = normalize(pos - spotPosition);
	vec3 LiSpot = spotColor * spotIntensity * spotLightAttenuation();
	float NLspot = dot(N, -Lspot);

	if (NLspot > 0)
	{
		vec3 specTexColor = texture(specularTexture, uv0).rgb;
        float shininess = alpha * 256.0f;

		float normalization = (shininess + 2.0) / (2.0 * 3.1415926535);
		vec3 V = normalize(cameraPos - pos);
		vec3 R = reflect(Lspot, N);
		float VR = pow(max(dot(V, R), 0.0f), shininess);
		
		shininess = (1 - alpha);
		float RF0 = pow((shininess - 1)/(shininess + 1), 2);
        float cosTheta = max(dot(N, V), 0.0);
        float fresnel = RF0 + (1 - RF0) * pow(1-cosTheta, 5);

		//vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * lightColor * NL;
		//vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * lightColor * NL;

		vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * LiSpot * NLspot;
		vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * LiSpot * NLspot;
		hdr += diffuse + specular;
	} 
	
	// Tone mapping
	vec3 ldr = hdr.rgb / (hdr.rgb + vec3(1.0));
	ldr = pow(ldr, vec3(1/2.2));
	
	outColor =  vec4(ldr, alpha);
}