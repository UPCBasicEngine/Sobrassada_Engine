#version 460

#extension GL_ARB_bindless_texture : require

uniform sampler2D positionTex; // G-Buffer positions
uniform mat4 invModel; // inverse model matrix of the cube
uniform sampler2D decalAlbedoTex;
in vec4 clipping;

in vec2 uv;

out vec4 fragColor;

void main()
{
    vec3 worldPos = texture(positionTex, clipping.xy/clipping.w*0.5+0.5).xyz;
    vec3 objPos = (invModel*vec4(worldPos, 1.0)).xyz;
    if (abs(objPos.x) > 0.5 || abs(objPos.y) > 0.5 || abs(objPos.z) > 0.5)
    discard;

    fragColor.rgb = texture(decalAlbedoTex, objPos.xy+0.5).rgb;
}