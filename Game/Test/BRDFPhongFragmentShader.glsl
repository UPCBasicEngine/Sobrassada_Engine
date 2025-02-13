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

    vec3 ambient = ambientIntensity * texColor;
    vec3 result = ambient;

    vec3 N = finalNormal;
    float NL = dot(N, lightDir);

    if (NL > 0) {
        vec3 diffuse = diffFactor * texColor * lightColor * lightIntensity * NL;

        vec3 specTexColor = texture(specularTexture, uv0).rgb;
        float shininess = alpha * 256.0f;

        vec3 V = normalize(cameraPos - pos);
        vec3 R = reflect(-lightDir, N);
        float VR = pow(max(dot(V, R), 0.0f), shininess);

        float RF0 = pow((shininess - 1) / (shininess + 1), 2);
        float cosTheta = max(dot(N, V), 0.0);
        float fresnel = RF0 + (1 - RF0) * pow(1 - cosTheta, 5);

        vec3 specular = specFactor * specTexColor * VR * lightColor * fresnel;

        result += diffuse + specular;
    }
    result = pow(result, vec3(1 / 2.2));

    outColor = vec4(result, alpha);
}
