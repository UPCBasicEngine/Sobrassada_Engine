#version 460 core
in vec2 uv0;

layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D ssaoInput;

uniform bool horizontal;

const int kernelSize = 5;
const float weights[kernelSize] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(ssaoInput, 0);
    float result = texture(ssaoInput, uv0).r * weights[0];
    
    for (int i = 1; i < kernelSize; ++i) {
        vec2 offset = horizontal ? vec2(texelSize.x * i, 0.0) : vec2(0.0, texelSize.y * i);
        result += texture(ssaoInput, uv0 + offset).r * weights[i];
        result += texture(ssaoInput, uv0 - offset).r * weights[i];
    }
    
    FragColor = vec4(result, 0.0, 0.0, 1.0);
}