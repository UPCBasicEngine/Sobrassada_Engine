#version 460

#extension GL_ARB_bindless_texture : require

uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D decalAlbedoTex;
uniform sampler2D decalMetallicTex;
uniform bool hasMetallic;
uniform sampler2D decalNormalTex;
uniform bool hasNormal;

in vec4 clipping;
in mat4 vInvModel;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 specular;
layout(location = 2) out vec4 gPosition;
layout(location = 3) out vec4 normalOut;

void main()
{
    vec2 uv = clipping.xy / clipping.w * 0.5 + 0.5;

    vec3 worldPos = texture(positionTex, uv).xyz;
    vec3 objPos = (vInvModel * vec4(worldPos, 1.0)).xyz;

    vec3 worldNormal = texture(normalTex, uv).xyz * 2.0 - 1.0;
    vec3 localNormal = normalize((vInvModel * vec4(worldNormal, 0.0)).xyz);

    float alignment = dot(localNormal, vec3(0.0, 0.0, 1.0));
    if (alignment < cos(radians(60.0))) discard;

    if (abs(objPos.x) > 0.5 || abs(objPos.y) > 0.5 || abs(objPos.z) > 0.5) discard;

    vec3 tangent = dFdx(worldPos);
    vec3 bitangent = dFdy(worldPos);
    vec3 normal = normalize(cross(tangent, bitangent));

    fragColor = texture(decalAlbedoTex, objPos.xy + 0.5);
    normalOut.xyz = (mat3(tangent, bitangent, normal) * (texture(decalNormalTex, objPos.xy + 0.5).xyz * 2.0 - 1.0));
}