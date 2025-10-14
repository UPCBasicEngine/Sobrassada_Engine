#version 430

struct PointLight
{
	vec4 position;		// xyz = position & w = range
	vec4 color;			// rgb = color & alpha = intensity 
};

struct SpotLight
{
	vec4 position;		// xyz = position & w = range
	vec4 color;			// rgb = color & alpha = intensity
	vec3 direction;	
	float innerAngle;
	float outerAngle;
	int spotShadowIndex;
    float radius;
    float anisotropy;
};

struct VolumetricArea
{
	vec4 position;		// xyz = position & w = type -> 0 AABB | 1 SPHERE
	vec4 size;			// if type == 1 -> radius = size.x
};

readonly layout(std430, binding = 4) buffer PointLights
{
	int pointLightsCount;
	PointLight pointLights[];
};

readonly layout(std430, binding = 5) buffer SpotLights
{
	int spotLightsCount;
	SpotLight spotLights[];
};

readonly layout(std430, binding = 8) buffer VolumeAreas
{
	int volumeAreaCount;
	VolumetricArea volumetricAreas[];
};

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

struct VisibleIndex {
	int index;
};

layout(std430, binding = 1) writeonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[];
} visibleLightIndicesBuffer;

layout(std430, binding = 3) writeonly buffer VisibleVolumetricAreaIndicesBuffer {
	VisibleIndex data[];
} visibleVolumetricAreaIndicesBuffer;

// Uniforms
uniform sampler2D depthMap;
uniform ivec2 screenSize;

// Shared values between all the threads in the group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared uint visibleAreaCount;
shared vec4 frustumPlanes[6];
// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visibleLightIndices[250];
shared int visibleAreaIndices[250];
shared mat4 viewProjection;


#define TILE_SIZE 16
layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main() {
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);
	ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
	uint index = tileID.y * tileNumber.x + tileID.x;

	if (gl_LocalInvocationIndex == 0) {
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
		visibleLightCount = 0;
		visibleAreaCount = 0;
		viewProjection = projMatrix * viewMatrix;
	}

	barrier();

	// Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
	float maxDepth, minDepth;
	vec2 text = vec2(location) / screenSize;
	float depth = texture(depthMap, text).r;
	// Linearize the depth value from depth buffer (must do this because we created it using projection)
	depth = (0.5 * projMatrix[3][2]) / (depth + 0.5 * projMatrix[2][2] - 0.5);

	// Convert depth to uint so we can do atomic min and max comparisons between the threads
	uint depthInt = floatBitsToUint(depth);
	atomicMin(minDepthInt, depthInt);
	atomicMax(maxDepthInt, depthInt);

	barrier();

	// Step 2: One thread should calculate the frustum planes to be used for this tile
	if (gl_LocalInvocationIndex == 0) {
		// Convert the min and max across the entire tile back to float
		minDepth = uintBitsToFloat(minDepthInt);
		maxDepth = uintBitsToFloat(maxDepthInt);

		// Steps based on tile sale
		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

		// Set up starting values for planes using steps and min and max z values
		frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
		frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
		frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		// Transform the first four planes
		for (uint i = 0; i < 4; i++) {
			frustumPlanes[i] *= viewProjection;
			frustumPlanes[i] /= length(frustumPlanes[i].xyz);
		}

		// Transform the depth planes
		frustumPlanes[4] *= viewMatrix;
		frustumPlanes[4] /= length(frustumPlanes[4].xyz);
		frustumPlanes[5] *= viewMatrix;
		frustumPlanes[5] /= length(frustumPlanes[5].xyz);
	}

	barrier();

	// Step 3: Cull lights.
	uint threadCount = TILE_SIZE * TILE_SIZE;
	uint passCount = (pointLightsCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++) {
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= pointLightsCount) {
			break;
		}

		vec4 position = vec4(pointLights[lightIndex].position.xyz, 1.0); 
		float radius = pointLights[lightIndex].position.w;

		// We check if the light exists in our frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++) {
			distance = dot(position, frustumPlanes[j]) + radius;

			// If one of the tests fails, then there is no intersection
			if (distance <= 0.0) {
				break;
			}
		}

		// If greater than zero, then it is a visible light
		if (distance > 0.0) {
			uint offset = atomicAdd(visibleLightCount, 1);
			visibleLightIndices[offset] = int(lightIndex);
		}
	}

	barrier();

	passCount = (spotLightsCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++) {
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= spotLightsCount) {
			break;
		}

		SpotLight light = spotLights[lightIndex];
		vec3 pos = light.position.xyz;
		float radius = light.position.w;

		vec3 dir = normalize(light.direction);
		float outerAngle = light.outerAngle;

		// (opcional) Aprox: bounding sphere test
		vec4 spherePos = vec4(pos, 1.0);
		float distance = 0.0;
		for (uint j = 0; j < 6; ++j) {
			distance = dot(spherePos, frustumPlanes[j]) + radius;
			if (distance <= 0.0)
				break;
		}

		if (distance > 0.0) {
			uint offset = atomicAdd(visibleLightCount, 1);
			// Para distinguir entre point y spot, por ejemplo: usar signo
			visibleLightIndices[offset] = int(0x80000000 | lightIndex); // usar MSB como flag
		}
	}

	// CALCULATING AREAS
	passCount = (volumeAreaCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++) {
		uint areaIndex = i * threadCount + gl_LocalInvocationIndex;
		if (areaIndex >= volumeAreaCount) {
			break;
		}

		VolumetricArea area = volumetricAreas[areaIndex];

		vec4 position  = vec4(area.position.xyz, 1.0);
	
		float radius = area.position.w == 0 ? max(max(area.size.x,area.size.y),area.size.z) : area.size.x;

		// We check if the area exists in our frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++) {
			distance = dot(position, frustumPlanes[j]) + radius;

			// If one of the tests fails, then there is no intersection
			if (distance <= 0.0) {
				break;
			}
		}

		if (distance > 0.0) {
			uint offset = atomicAdd(visibleAreaCount, 1);
			visibleAreaIndices[offset] = int(areaIndex);
		}
	}

	barrier();

	// One thread should fill the global light buffer
	if (gl_LocalInvocationIndex == 0) {
		uint offset = index * 250; // Determine bosition in global buffer
		for (uint i = 0; i < visibleLightCount; i++) {
			visibleLightIndicesBuffer.data[offset + i].index = visibleLightIndices[i];
		}

		if (visibleLightCount != 250) {
			visibleLightIndicesBuffer.data[offset + visibleLightCount].index = -1;
		}

		for (uint i = 0; i < visibleAreaCount; i++) {
			visibleVolumetricAreaIndicesBuffer.data[offset + i].index = visibleAreaIndices[i];
		}

		if (visibleAreaCount != 250) {
			visibleVolumetricAreaIndicesBuffer.data[offset + visibleAreaCount].index = -1;
		}
	}
}