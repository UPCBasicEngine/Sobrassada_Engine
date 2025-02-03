#version 460

out vec4 color;
in vec3 pos;
in vec2 uv0;
in vec3 normal;
out vec4 outColor;

layout(binding=0) uniform sampler2D diffuseTexture;
layout(binding=1) uniform sampler2D specularTexture;

uniform vec3 lightColor;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float ambientIntensity;

void main()
{
	vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
   
	vec3 ambient = ambientIntensity * texColor;
	vec3 result = ambient;
	vec3 N = normalize(normal);
   	vec3 L = normalize(lightDir);
	
	float NL = dot(N, L);
	if(NL > 0){
		vec3 specTexColor = texture(specularTexture, uv0).rgb;
		float alpha = texture(specularTexture, uv0).a;
		float shininess = alpha*256;
			
		float normalization = (shininess + 2.0) / (2.0 * 3.1415926535);	
		vec3 V = normalize(cameraPos - pos);
		vec3 R = reflect(-L, N);
		float VR = pow(max(dot(V, R), 0.0f), shininess);
			
		//Fresnel
		float RF0 = pow((shininess - 1)/(shininess + 1), 2);
		float cosTheta = max(dot(N, V), 0.0);
		float fresnel = RF0 + (1 - RF0) * pow(1-cosTheta, 5);

		vec3 diffuse = (1.0 - RF0) / 3.1415926535 * texColor * lightColor * NL;
		vec3 specular = specTexColor * VR * lightColor * fresnel * normalization;

		result = ambient + specular + diffuse;
	}
	
	result = pow(result, vec3(1/2.2));

	outColor =  vec4(result, 1.0f);
}