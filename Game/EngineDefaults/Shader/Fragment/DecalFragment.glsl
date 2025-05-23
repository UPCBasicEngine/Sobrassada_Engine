#version 460

#extension GL_ARB_bindless_texture : require

uniform sampler2D positionTex;
uniform mat4 invModel;
uniform sampler2D decalAlbedoTex;
uniform sampler2D decalNormalTex;
in vec4 clipping;

in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 specular;
layout(location = 2) out vec4 gPosition;
layout(location = 3) out vec4 normalOut;

void main()
{
    vec3 worldPos = texture(positionTex, clipping.xy / clipping.w * 0.5 + 0.5).xyz;
    vec3 objPos = (invModel * vec4(worldPos, 1.0)).xyz;

    vec3 tangent = dFdx(worldPos);
    vec3 bitangent = dFdy(worldPos);
    vec3 normal = normalize(cross(tangent, bitangent));

    if (abs(objPos.x) > 0.5 || abs(objPos.y) > 0.5 || abs(objPos.z) > 0.5) discard;

    fragColor = texture(decalAlbedoTex, objPos.xy + 0.5);
    normalOut.xyz = (mat3(tangent, bitangent, normal) * (texture(decalNormalTex, objPos.xy + 0.5).xyz * 2.0 - 1.0));
}