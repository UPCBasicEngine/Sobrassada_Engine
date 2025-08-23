#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 maskTexture;

layout(location = 5) uniform float intensity;     
layout(location = 6) uniform float pulseSpeed;     
layout(location = 7) uniform float time;           
layout(location = 8) uniform float noiseTiling;
layout(location = 9) uniform float noiseSpeed;

float radialDist(vec2 uv) {
    return length(uv - 0.5) / 0.7071;
}

void main()
{
    const float centerDistance = radialDist(uv0);
    const float pulse = pow(sin(time * pulseSpeed) * 0.5f + 0.5f, 4.0f);

    float threshold = mix(1.1f, 0.6f, intensity); 

    const vec2 centerDir    = normalize(uv0 - vec2(0.5, 0.5));
    const vec2 noiseVec = uv0 * noiseTiling + centerDir * time * noiseSpeed;
    const float noise = texture(sampler2D(maskTexture), noiseVec).r;

    threshold += (noise - 0.5) * 0.3 * intensity;

    const float edge = smoothstep(threshold - 0.2f, threshold, centerDistance);
    const float strength = clamp(intensity + pulse * intensity * 0.5, 0.0, 1.0);

    vec4 col = vec4(inputColor, 1.0f);

    col.a = edge * strength;
    outColor = col;        
}
