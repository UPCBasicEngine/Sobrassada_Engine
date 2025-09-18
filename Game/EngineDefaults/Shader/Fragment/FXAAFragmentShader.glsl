#version 460

in vec2 uv0;

layout(binding=0) uniform sampler2D lastDrawTexture;

layout(location=0) uniform bool showBorders;
layout(location=1) uniform float globalThreshold;
layout(location=2) uniform float localThreshold;
layout(location=3) uniform bool enableFXAA;

out vec4 fragColor;

struct LuminanceData
{
    float middle;
    float north;
    float east;
    float south;
    float west;
    float northEast;
    float southEast;
    float southWest;
    float northWest;
    float contrast;
};

struct EdgeData 
{
    bool isHorizontal;
    vec2 pixelStep;
    float oppositeLuminance;
    float gradient;
};

float luma(const vec4 color)
{
    return color.g;
}

float getMax(float values[5])
{
    float maxValue = 0;
    for (int i = 0; i < 5; ++i) {
        if (maxValue < values[i]) maxValue = values[i];
    }

    return maxValue;
}

float getMin(float values[5])
{
    float minValue = 1;
    for (int i = 0; i < 5; ++i) {
        if (minValue > values[i]) minValue = values[i];
    }

    return minValue;
}

vec2 ComputeBlendDirection(float middle, float neighbour, float oppositeNeighbour)
{
    vec2 pixelStep;
    
    return pixelStep;
}

float ComputePixelBlendFactor(LuminanceData l)
{
    float average = 2.0f * (l.north + l.east + l.south + l.west) + l.northEast + l.southEast + l.southWest + l.northWest;
    average /= 12.0f;

    float pixelBlendFactor = abs(l.middle - average) / l.contrast;
    pixelBlendFactor = smoothstep(0.0f, 1.0f, clamp(pixelBlendFactor, 0.0f, 1.0f));
    return pixelBlendFactor * pixelBlendFactor;
}

EdgeData ComputeEdgeDirection(LuminanceData l)
{
    EdgeData e;

    // Edge blend direction
    float verticalEdge = abs((0.25 * l.northWest) + (-0.5 * l.north) + (0.25 * l.northEast)) +        
                        abs((0.50 * l.west ) + (-1.0 * l.middle) + (0.50 * l.east )) + 
                        abs((0.25 * l.southWest) + (-0.5 * l.south) + (0.25 * l.southEast)); 
                        
    float horizontalEdge = abs((0.25 * l.northWest) + (-0.5 * l.west) + (0.25 * l.southWest)) + 
                        abs((0.50 * l.north ) + (-1.0 * l.middle) + (0.50 * l.south)) + 
                        abs((0.25 * l.northEast) + (-0.5 * l.east) + (0.25 * l.southEast));
    
    e.isHorizontal = horizontalEdge >= verticalEdge; 

    // Compute edge direction
    float neighbour = e.isHorizontal ? l.north : l.east;
    float oppositeNeighbour = e.isHorizontal ? l.south : l.west;

    float pGradient = abs(neighbour - l.middle);
    float nGradient = abs(oppositeNeighbour - l.middle);
    if (pGradient >= nGradient)
    {
        e.pixelStep = e.isHorizontal ? vec2(0.0f, 1.0f / textureSize(lastDrawTexture, 0).y) : vec2(1.0f / textureSize(lastDrawTexture, 0).x, 0.0f);
        e.oppositeLuminance = neighbour;
        e.gradient = pGradient;
    }
    else
    {
        e.pixelStep = e.isHorizontal ? vec2(0.0f, -1.0f / textureSize(lastDrawTexture, 0).y) : vec2(-1.0f / textureSize(lastDrawTexture, 0).x, 0.0f);
        e.oppositeLuminance = oppositeNeighbour;
        e.gradient = nGradient;
    }

    return e;
}

float ComputeEdgeBlendFactor(LuminanceData l, EdgeData e)
{
    vec2 currentUv = uv0;

    currentUv += e.pixelStep * 0.5f;

    return 0;
}

vec3 ApplyFXAA(LuminanceData l) 
{
    float pixelBlendFactor = ComputePixelBlendFactor(l);
    EdgeData e = ComputeEdgeDirection(l);
    float edgeBlendFactor = ComputeEdgeBlendFactor(l, e);

    if (showBorders)
    {
        vec3 color = e.isHorizontal ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
        return color;
    }
    else
    {
        return texture(lastDrawTexture, uv0 + e.pixelStep * pixelBlendFactor).rgb;
    }
}

void main()
{
    if (!enableFXAA)  // If disable, just pass-through. Needed for drawing to screen framebuffer anyway
    {
        fragColor = texture(lastDrawTexture, uv0);
        return;
    }

    LuminanceData l;

    // Immediate neighbours
    l.middle = luma(texture(lastDrawTexture, uv0));
    l.north = luma(textureOffset(lastDrawTexture, uv0, ivec2(0, 1)));
    l.east = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, 0)));
    l.south = luma(textureOffset(lastDrawTexture, uv0, ivec2(0, -1)));
    l.west = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, 0)));

    // Diagonal neighbours
    l.northEast = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, 1)));
    l.southEast = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, -1)));
    l.southWest = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, -1)));
    l.northWest = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, 1)));
    
    float luminance[5] = float[5](l.middle, l.north, l.east, l.south, l.west);
    float maxValue = getMax(luminance);
    float minValue = getMin(luminance);
    l.contrast = maxValue - minValue;

    if(l.contrast < max(localThreshold * maxValue, globalThreshold))
    {
        fragColor = texture(lastDrawTexture, uv0);
        //fragColor = vec4(vec3(0.0f), 1.0f);
    }
    else
    {
        fragColor = vec4(ApplyFXAA(l), 1.0f);
    } 
}