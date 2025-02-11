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

uniform vec3 lightColor;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float ambientIntensity;

mat3 CreateTBN(const vec3 N, const vec4 T)
{
    vec3 Tn = normalize(vec3(T));          	 // Tangent
    vec3 B = T.w * normalize(cross(N, Tn));  // Bitangent, with the direction (T.w)
    return mat3(Tn, B, normalize(N));
}

void main()
{
	//Texture sample and remap normal [0 - 1] to [-1 - 1]
	vec3 texNormal = texture(normal_map, uv0).xyz;
	texNormal = normalize(texNormal * 2.0 - 1.0);

	//Transform vector to world space
	mat3 tbn = CreateTBN(normal, tangent);
	vec3 finalNormal = normalize(tbn * texNormal);

	vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
	float alpha = texture(specularTexture, uv0).a;

	vec3 ambient = ambientIntensity * texColor;
	vec3 result = ambient;

	vec3 N = normal;
   	vec3 L = normalize(lightDir);
	
	float NL = dot(N, -L);
	if(NL > 0){
   		vec3 specTexColor = texture(specularTexture, uv0).rgb;
        float shininess = alpha * 256.0f;

		float normalization = (shininess + 2.0) / (2.0 * 3.1415926535);
		vec3 V = normalize(cameraPos - pos);
		vec3 R = reflect(L, N);
		float VR = pow(max(dot(V, R), 0.0f), shininess);
		
		float RF0 = pow((shininess - 1)/(shininess + 1), 2);
                float cosTheta = max(dot(N, V), 0.0);
                float fresnel = RF0 + (1 - RF0) * pow(1-cosTheta, 5);

		vec3 diffuse = (1.0 - RF0) / 3.1415926535 * diffFactor * texColor * lightColor * NL;
		vec3 specular = normalization * specFactor * specTexColor * VR * lightColor * fresnel;
		result = ambient + diffuse + specular;
	}
	
	result = pow(result, vec3(1/2.2));
	
	outColor =  vec4(result, alpha);
}
