#version 460

#define EDGE_STEP_COUNT 12
#define EDGE_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 3, 3, 4

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

    float positiveGradient = abs(neighbour - l.middle);
    float negativeGradient = abs(oppositeNeighbour - l.middle);
    if (positiveGradient >= negativeGradient)
    {
        e.pixelStep = e.isHorizontal ? vec2(0.0f, 1.0f / textureSize(lastDrawTexture, 0).y) : vec2(1.0f / textureSize(lastDrawTexture, 0).x, 0.0f);
        e.oppositeLuminance = neighbour;
        e.gradient = positiveGradient;
    }
    else
    {
        e.pixelStep = e.isHorizontal ? vec2(0.0f, -1.0f / textureSize(lastDrawTexture, 0).y) : vec2(-1.0f / textureSize(lastDrawTexture, 0).x, 0.0f);
        e.oppositeLuminance = oppositeNeighbour;
        e.gradient = negativeGradient;
    }

    return e;
}

float ComputeEdgeBlendFactor(LuminanceData l, EdgeData e)
{
    const float edgeSteps[EDGE_STEP_COUNT] = { EDGE_STEPS };
    vec2 startUv = uv0;

    // Go the the middle point between the border pixels
    startUv += e.pixelStep * 0.5f;

    vec2 edgeStep;
    if (e.isHorizontal) edgeStep = vec2(1.0f / textureSize(lastDrawTexture, 0).x, 0.0f);
    else edgeStep = vec2(0.0f, 1.0f / textureSize(lastDrawTexture, 0).y);

    float startEdgeLuminance = (l.middle + e.oppositeLuminance) * 0.5f;
    float gradientThreshold = e.gradient * 0.25f;

    vec2 positiveUv = startUv;
    float positiveLuminance = startEdgeLuminance;
    for (int i = 0; i < EDGE_STEP_COUNT; ++i) 
    {
        positiveUv += edgeStep * edgeSteps[i];
        positiveLuminance = luma(texture(lastDrawTexture, positiveUv)) - startEdgeLuminance;
        if (abs(positiveLuminance) >= gradientThreshold) break;  // If bigger than threshold, we are at the end of the border
    }

    vec2 negativeUv = startUv;
    float negativeLuminance = startEdgeLuminance;
    for (int i = 0; i < EDGE_STEP_COUNT; ++i) 
    {
        negativeUv -= edgeStep * edgeSteps[i];
        negativeLuminance = luma(texture(lastDrawTexture, negativeUv)) - startEdgeLuminance;
        if (abs(negativeLuminance) >= gradientThreshold) break;  // If bigger than threshold, we are at the end of the border
    }

    float positiveDistance = 0.0f;
    float negativeDistance = 0.0f;
    if (e.isHorizontal)
    {
        positiveDistance = positiveUv.x - startUv.x;
        negativeDistance = startUv.x - negativeUv.x;
    }
    else
    {
        positiveDistance = positiveUv.y - startUv.y;
        negativeDistance = startUv.y - negativeUv.y;
    }

    bool deltaSign = false;
    float shortestDistance = 0.0f;
    if (positiveDistance <= negativeDistance)
    {
        shortestDistance = positiveDistance;
        deltaSign = positiveLuminance >= 0;
    }
    else
    {
        shortestDistance = negativeDistance;
        deltaSign = negativeLuminance >= 0;
    }

    // Only blend in one direction of the border
    if (deltaSign == (l.middle - startEdgeLuminance >= 0)) return 0;
    else return 0.5f - shortestDistance / (positiveDistance + negativeDistance);
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
        return texture(lastDrawTexture, uv0 + e.pixelStep * max(pixelBlendFactor, edgeBlendFactor)).rgb;
    }
}

void main()
{
    if (!enableFXAA)  // If disabled, just pass-through. Needed for drawing to screen framebuffer anyway
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
    }
    else
    {
        fragColor = vec4(ApplyFXAA(l), 1.0f);
    } 
}