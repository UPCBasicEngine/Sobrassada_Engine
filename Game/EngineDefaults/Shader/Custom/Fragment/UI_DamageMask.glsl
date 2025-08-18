#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 maskTexture;

layout(location = 5) uniform float intensity;      // 0 = healthy, 1 = critical
layout(location = 6) uniform float pulseSpeed;     // how fast the heartbeat is (Hz-ish)
layout(location = 7) uniform float time;           // global time in seconds
layout(location = 8) uniform float noiseTiling;
layout(location = 9) uniform float noiseSpeed;

float radialDist(vec2 uv) {
    return length(uv - 0.5) / 0.7071;
}

void main()
{
    float dist = radialDist(uv0);
    
    float pulse = pow(sin(time * pulseSpeed) * 0.5f + 0.5f, 4.0f);

    // edge cutoff threshold (closer to center as injury increases)
    float threshold = mix(1.1f, 0.6f, intensity); 

    vec2 centerDir    = normalize(uv0 - vec2(0.5, 0.5));
    vec2 noiseVec = uv0 * noiseTiling + centerDir * time * noiseSpeed;
    float noise = texture(sampler2D(maskTexture), noiseVec).r;

    threshold += (noise - 0.5) * 0.3 * intensity;

    float edge = smoothstep(threshold - 0.2f, threshold, dist);

    float strength = clamp(intensity + pulse * intensity * 0.5, 0.0, 1.0);

    vec4 col = vec4(inputColor, 1.0f);

    col.a = edge * strength;
    outColor = col;        
}
