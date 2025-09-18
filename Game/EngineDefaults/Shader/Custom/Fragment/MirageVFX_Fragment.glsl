#version 460

layout(location = 8) uniform float animationTimer;
layout(location = 9) uniform float sharpness;

in vec2 uv;

out vec4 fragColor;

vec2 hash( vec2 p )
{
    p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
    return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

    vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
    vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
    vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}

float smoothnoise( in vec2 p ){    
    p += animationTimer*0.05;
    float f = 0.0;
    
    p *= 5.0;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise( p ); p = m*p;
    f += 0.2500*noise( p ); p = m*p;
    f += 0.1250*noise( p ); p = m*p;
    f += 0.0625*noise( p ); p = m*p;

    return 0.5 + 0.5*f;
}

vec3 colorRamp(float x){
    float thresholds[4] = float[4](0.02, 0.258, 0.433, 0.649);
    vec3 colors[4] = vec3[4](
        vec3(0, 0, 0), 
        vec3(0.188, 0.357, 0.733), 
        vec3(0.153, 0.941, 0.957), 
        vec3(1.0, 1.0, 1.0)
    );
    if(x < thresholds[0]) return colors[0];
    if(x >= thresholds[3]) return colors[3];

    int i;
    for(i = 1; i < 4; ++i){
        if(x < thresholds[i]) break;
    }
    float numerator = x - thresholds[i-1];
    float denominator = thresholds[i] - thresholds[i-1];
    float r = numerator / denominator;
    return colors[i-1] * (1.0 - r) + colors[i] * r;
}

void main()
{
    vec2 texcoord_read = vec2((1 - uv.y) * 10.0f, animationTimer);
    float read = smoothnoise(texcoord_read);
    float read2 = pow(read, 2.5);
    float value = clamp((1 - uv.x) * (read2 + 0.39), 0.0, 1.0);

    fragColor = vec4(colorRamp(value), 1.0) * value;
}