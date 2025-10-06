#version 460

layout(binding=0) uniform sampler2D myTexture;

uniform layout(location = 0) bool horizontal;

uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

in vec2 uv0;

out vec4 FragColor;

void main()
{
    vec2 textureOffset = 1.0 / textureSize(myTexture, 0);
    vec3 result = texture(myTexture, uv0).rgb * weight[0];

    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(myTexture, uv0 + vec2(textureOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(myTexture, uv0 - vec2(textureOffset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(myTexture, uv0 + vec2(0.0, textureOffset.y * i)).rgb * weight[i];
            result += texture(myTexture, uv0 - vec2(0.0, textureOffset.y * i)).rgb * weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}