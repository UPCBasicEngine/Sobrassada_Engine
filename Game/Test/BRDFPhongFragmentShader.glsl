#version 460

out vec4 color;
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

layout(location = 3) uniform vec3 spotColor;
layout(location = 4) uniform vec3 spotPosition;
layout(location = 5) uniform float spotIntensity;
layout(location = 6) uniform float spotRange;

float lightAttenuation() 
{
	float distance = length(pos - spotPosition);

	return pow(max(1 - pow((distance / spotRange), 4), 0), 2) / (pow(distance, 2) + 1);
}

void main()
{
	vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
	float alpha = texture(specularTexture, uv0).a;

	vec3 ambient = ambientIntensity * texColor;
	vec3 result = ambient;

	vec3 N = normalize(normal);
   	vec3 L = normalize(lightDir);

	vec3 Lspot = normalize(pos - spotPosition);
	vec3 Li = spotColor * spotIntensity * lightAttenuation();
	float NLspot = dot(N, Lspot);
	
	float NL = dot(N, -L);

	if (NLspot > 0)
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

		//vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * lightColor * NL;
		//vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * lightColor * NL;

		vec3 diffuse = (diffFactor * (1 - RF0)) / 3.1415926535 * texColor * Li * NLspot;
		vec3 specular = normalization * specFactor * specTexColor * VR * fresnel * Li * NLspot;
		result = ambient + diffuse + specular;
	}
	
	result = pow(result, vec3(1/2.2));
	
	outColor =  vec4(result, alpha);
}