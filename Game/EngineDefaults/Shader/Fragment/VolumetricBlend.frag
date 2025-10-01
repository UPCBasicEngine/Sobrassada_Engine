#version 460

in vec2 uv0;
out vec4 FragColor;
uniform layout(location = 0) sampler2D volumetricTexture;
uniform layout(location = 1) sampler2D sceneTexture;

void main()
{
    vec4 volumetricColor = texture(volumetricTexture, uv0);
    vec4 sceneColor = texture(sceneTexture, uv0);

    FragColor = sceneColor + volumetricColor;
}