#version 460

#extension GL_ARB_bindless_texture : require

out vec4 frag_color;
in vec2 uv0;
flat in int instance_index;

struct Material
{
    vec4 diffColor;
    vec3 specColor;
    float shininess;
    bool shininessInAlpha;
    float metallicFactor;
    float roughnessFactor;
    uvec2 diffuseTex;
    uvec2 specularTex;
    uvec2 metallicTex;
    uvec2 normalTex;
};

readonly layout(std430, binding = 11) buffer Materials {
    Material materials[];
};

void main()
{
    const Material mat = materials[instance_index];

    vec4 diffuseColor = texture(sampler2D(mat.diffuseTex), uv0);
    vec4 color = vec4(pow(diffuseColor.rgb, vec3(2.2)), diffuseColor.a);
    if(mat.diffColor.a < 0.1) discard;
    frag_color = vec4(color.rgb, mat.diffColor.a);
}