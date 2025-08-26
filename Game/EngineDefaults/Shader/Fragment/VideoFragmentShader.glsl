#version 460
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D videoTexture;

void main()
{
    FragColor = texture(videoTexture, TexCoord);
}