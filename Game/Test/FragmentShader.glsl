#version 460
out vec4 color;
in vec3 normal;
in vec2 uv0;
in vec4 tangent;

uniform sampler2D normal_map;

layout(binding=0) uniform sampler2D myTexture;

void main()
{
	//Texture sample and remap normal [0 - 1] to [-1 - 1]
	vec3 texNormal = texture(normal_map, uv0).xyz;
	texNormal = normalize(texNormal * 2.0 - 1.0);

	//Transform vector to world space
	mat3 tbn = CreateTBN(normal, tangent);
	vec3 finalNormal = normalize(tbn * texNormal);

	//TODO: PUT THE NORMAL CALCULATE IN THE LIGHTS CALCULATIONS

	color = texture2D(myTexture, uv0);
}


mat3 CreateTBN(const vec3 N, const vec4 T)
{
    vec3 Tn = normalize(vec3(T));          	 // Tangent
    vec3 B = T.w * normalize(cross(N, Tn));  // Bitangent, with the direction (T.w)
    return mat3(Tn, B, normalize(N));
}
