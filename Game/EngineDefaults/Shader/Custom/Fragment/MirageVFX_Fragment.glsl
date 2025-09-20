#version 460

layout(location = 4) uniform float animationTimer;

in vec2 uv;

out vec4 fragColor;

vec2 hash( vec2 p )
{
    p = vec2(dot(p,vec2(127.1,311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;

    vec2  i = floor(p + (p.x + p.y) * K1);
    vec2  a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x); 
    vec2  o = vec2(m, 1.0 - m);
    vec2  b = a - o + K2;
    vec2  c = a - 1.0 + 2.0 * K2;
    vec3  h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    vec3  n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i+o)), dot(c, hash(i+1.0)));
    return dot(n, vec3(70.0));
}

float smoothnoise( in vec2 p ){    
    p += animationTimer * 0.05;
    float f = 0.0;
    
    p *= 5.0;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    f  = 0.5000 * noise(p); p = m * p;
    f += 0.2500 * noise(p); p = m * p;
    f += 0.1250 * noise(p); p = m * p;
    f += 0.0625 * noise(p); p = m * p;

    return 0.5 + 0.5 * f;
}

vec3 colorRamp(float x){
    vec3 color1 = vec3(0, 0, 0);
    vec3 color2 = vec3(0.188, 0.357, 0.733);
    vec3 color3 = vec3(0.153, 0.941, 0.957);
    vec3 color4 = vec3(1.0);

    if(x < 0.02) return color1;
    else if(x < 0.258) return mix(color1, color2, (x - 0.02)/(0.258 - 0.02));
    else if(x < 0.433) return mix(color2, color3, (x - 0.258)/(0.433 - 0.258));
    else if(x < 0.649) return mix(color3, color4, (x - 0.433)/(0.649 - 0.433));
    else return color4;
}

void main()
{
    vec2 texcoord_read = vec2((1 - uv.y) * 10.0f, animationTimer);
    float read = smoothnoise(texcoord_read);
    float read2 = pow(read, 2.5);
    float value = clamp((1 - uv.x) * (read2 + 0.39), 0.0, 1.0);

    fragColor = vec4(colorRamp(value), 1.0) * value;
}