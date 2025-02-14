#include "ResourceMesh.h"

#include <SDL_assert.h>
#include <glew.h>
#include <Math/float4x4.h>
#include <Math/float2.h>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

#include "Application.h"
#include "CameraModule.h"
#include "ResourceMaterial.h"

ResourceMesh::ResourceMesh(UID uid): Resource(uid, ResourceType::Mesh)
{
    aabb.SetNegativeInfinity();
}

ResourceMesh::~ResourceMesh()
{
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vao);
}

void ResourceMesh::LoadVBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive)
{
	// Getting required size for VBO buffer
	const auto& positionIterator = inPrimitive.attributes.find("POSITION");
	const auto& textureIterator = inPrimitive.attributes.find("TEXCOORD_0");
    const auto &normalIterator   = inPrimitive.attributes.find("NORMAL");
    const auto &tangentIterator  = inPrimitive.attributes.find("TANGENT");
	
	if (positionIterator != inPrimitive.attributes.end()) {
		const tinygltf::Accessor& positionAccessor = inModel.accessors[positionIterator->second];
		vertexCount = (int)positionAccessor.count;
	}

	if (textureIterator != inPrimitive.attributes.end()) {
		const tinygltf::Accessor& textureAccessor = inModel.accessors[textureIterator->second];
		textureCoordCount = (int)textureAccessor.count;
	}

	if (normalIterator != inPrimitive.attributes.end())
    {
		const tinygltf::Accessor &normalAccessor = inModel.accessors[normalIterator->second];
        normalCoordCount = (int)normalAccessor.count;
	}

	if (tangentIterator != inPrimitive.attributes.end())
    {
		const tinygltf::Accessor &tangentAccesor = inModel.accessors[tangentIterator->second];
		tangentCoordCount = (int)tangentAccesor.count;
    }

	if ((vertexCount + textureCoordCount + normalCoordCount + tangentCoordCount) > 0)
	{
		unsigned int bufferSize = (sizeof(float) * 3 * vertexCount) + (sizeof(float) * 2 * textureCoordCount) + (sizeof(float) * 3 * normalCoordCount) + (sizeof(float) * 4 * tangentCoordCount);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
	}

	// Loading position vertices to VBO
	if (vertexCount > 0)
	{
		const tinygltf::Accessor& positionAccessor = inModel.accessors[positionIterator->second];

		SDL_assert(positionAccessor.type == TINYGLTF_TYPE_VEC3);
		SDL_assert(positionAccessor.componentType == GL_FLOAT);

		//maximumPosition = float3((float)positionAccessor.maxValues[0], (float)positionAccessor.maxValues[1], (float)positionAccessor.maxValues[2]);
		//minimumPosition = float3((float)positionAccessor.minValues[0], (float)positionAccessor.minValues[1], (float)positionAccessor.minValues[2]);
		const float3 maximumPosition = float3((float)positionAccessor.maxValues[0], (float)positionAccessor.maxValues[1], (float)positionAccessor.maxValues[2]);
		const float3 minimumPosition = float3((float)positionAccessor.minValues[0], (float)positionAccessor.minValues[1], (float)positionAccessor.minValues[2]);
		aabb = AABB(minimumPosition, maximumPosition);

		const tinygltf::BufferView& positionBufferView = inModel.bufferViews[positionAccessor.bufferView];
		const tinygltf::Buffer& positionBuffer = inModel.buffers[positionBufferView.buffer];

		const unsigned char* bufferStart = &(positionBuffer.data[positionAccessor.byteOffset + positionBufferView.byteOffset]);
		
		float3* ptr = reinterpret_cast<float3*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * vertexCount, GL_MAP_WRITE_BIT));

		for (size_t i = 0; i < positionAccessor.count; ++i)
		{
			ptr[i] = *reinterpret_cast<const float3*>(bufferStart);

			// bufferView.byteStride == 0 -> Only positions inside buffer, which then the stride becomes space between vertices -> sizeof(float) * 3.
			bufferStart += positionBufferView.byteStride == 0 ? sizeof(float) * 3 : positionBufferView.byteStride;
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	// Loading texture coordinates to VBO, Separated array;
	if (textureCoordCount > 0)
	{
		const tinygltf::Accessor& textureAccessor = inModel.accessors[textureIterator->second];

		SDL_assert(textureAccessor.type == TINYGLTF_TYPE_VEC2);
		SDL_assert(textureAccessor.componentType == GL_FLOAT);

		const tinygltf::BufferView& textureBufferView = inModel.bufferViews[textureAccessor.bufferView];
		const tinygltf::Buffer& textureBuffer = inModel.buffers[textureBufferView.buffer];

		const unsigned char* bufferStart = &(textureBuffer.data[textureAccessor.byteOffset + textureBufferView.byteOffset]);

		float2* ptr = reinterpret_cast<float2*>(glMapBufferRange(GL_ARRAY_BUFFER, sizeof(float) * 3 * vertexCount, sizeof(float) * 2 * textureCoordCount, GL_MAP_WRITE_BIT));

		for (size_t i = 0; i < textureAccessor.count; ++i)
		{
			ptr[i] = *reinterpret_cast<const float2*>(bufferStart);

			// bufferView.byteStride == 0 -> Only positions inside buffer, which then the stride becomes space between vertices -> sizeof(float) * 3.
			bufferStart += textureBufferView.byteStride == 0 ? sizeof(float) * 2 : textureBufferView.byteStride;
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	// Loading normal vertices to VBO
	if (normalCoordCount > 0)
    {
		const tinygltf::Accessor &normalAccessor = inModel.accessors[normalIterator->second];

		SDL_assert(normalAccessor.type == TINYGLTF_TYPE_VEC3);
		SDL_assert(normalAccessor.componentType == GL_FLOAT);

        const tinygltf::BufferView &normalBufferView = inModel.bufferViews[normalAccessor.bufferView];
        const tinygltf::Buffer &normalBuffer         = inModel.buffers[normalBufferView.buffer];

        const unsigned char *bufferStart =
            &(normalBuffer.data[normalAccessor.byteOffset + normalBufferView.byteOffset]);

        float3 *ptr = reinterpret_cast<float3*>(glMapBufferRange(
            GL_ARRAY_BUFFER, sizeof(float) * 3 * vertexCount + sizeof(float) * 2 * textureCoordCount,
            sizeof(float) * 3 * normalCoordCount, GL_MAP_WRITE_BIT
        ));

        for (size_t i = 0; i < normalAccessor.count; ++i)
		{
            ptr[i] = *reinterpret_cast<const float3*>(bufferStart);

            // bufferView.byteStride == 0 -> Only positions inside buffer, which then the stride becomes
            // space between vertices -> sizeof(float) * 3.
            bufferStart += normalBufferView.byteStride == 0 ? sizeof(float) * 3 : normalBufferView.byteStride;
		}

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

	if (tangentCoordCount > 0)
    {
        const tinygltf::Accessor &tangentAccessor = inModel.accessors[tangentIterator->second];

        SDL_assert(tangentAccessor.type == TINYGLTF_TYPE_VEC4);
        SDL_assert(tangentAccessor.componentType == GL_FLOAT);

        const tinygltf::BufferView &tangentBufferView = inModel.bufferViews[tangentAccessor.bufferView];
        const tinygltf::Buffer &tangentBuffer         = inModel.buffers[tangentBufferView.buffer];

        const unsigned char *bufferStart =
            &(tangentBuffer.data[tangentAccessor.byteOffset + tangentBufferView.byteOffset]);

        float4 *ptr = reinterpret_cast<float4 *>(glMapBufferRange(
            GL_ARRAY_BUFFER, sizeof(float) * 3 * vertexCount + sizeof(float) * 2 * textureCoordCount +
                sizeof(float) * 3 * normalCoordCount,
            sizeof(float) * 4 * tangentCoordCount, GL_MAP_WRITE_BIT
        ));

        for (size_t i = 0; i < tangentAccessor.count; ++i)
        {
            ptr[i]       = *reinterpret_cast<const float4 *>(bufferStart);

            // bufferView.byteStride == 0 -> Only positions inside buffer, which then the stride becomes
            // space between vertices -> sizeof(float) * 4.
            bufferStart += tangentBufferView.byteStride == 0 ? sizeof(float) * 4 : tangentBufferView.byteStride;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
}

void ResourceMesh::LoadEBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive)
{
	if (inPrimitive.indices >= 0)
	{
		const tinygltf::Accessor& indexAccessor = inModel.accessors[inPrimitive.indices];
		const tinygltf::BufferView& indexBufferView = inModel.bufferViews[indexAccessor.bufferView];
		const tinygltf::Buffer& indexBuffer = inModel.buffers[indexBufferView.buffer];

		indexCount = (unsigned int)indexAccessor.count;

		const unsigned char* bufferStart = &(indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexCount, nullptr, GL_STATIC_DRAW);
		unsigned int* ptr = reinterpret_cast<unsigned int*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
		
		if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
		{
			const unsigned int* bufferInd = reinterpret_cast<const unsigned int*>(bufferStart);
			for (uint32_t i = 0; i < indexCount; ++i) ptr[i] = bufferInd[i];
		}
		if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
		{
			const unsigned short* bufferInd = reinterpret_cast<const unsigned short*>(bufferStart);
			for (unsigned short i = 0; i < indexCount; ++i) ptr[i] = bufferInd[i];
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}
}

void ResourceMesh::CreateVAO()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// IN CASE WE ARE USING INDICES FOR RENDERING
	if(indexCount > 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// SETTING TEXTURE COORDINATES IF LOADED
	if (textureCoordCount > 0)
	{
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * vertexCount));

		glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)((sizeof(float) * 3 * vertexCount) + (sizeof(float) * 2 * textureCoordCount)));
	}
    else
    {
		glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)(sizeof(float) * 3 * vertexCount));
	}

	// SETTING TANGENT COORDINATES IF LOADED
    if (tangentCoordCount > 0)
    {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(
            3, 4, GL_FLOAT, GL_FALSE, 0,
            (void *)(sizeof(float) * 3 * vertexCount + sizeof(float) * 2 * textureCoordCount +
                     sizeof(float) * 3 * normalCoordCount)
        );
    }

	glBindVertexArray(0);
}

void ResourceMesh::Render(int program, float4x4 &modelMatrix, unsigned int cameraUBO, ResourceMaterial* material)
{
	glUseProgram(program);
	
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
    glUniformBlockBinding(program, blockIdx, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    glUniformMatrix4fv(2, 1, GL_TRUE, &modelMatrix[0][0]);

	float3 lightDir = float3(-1.0f, -0.3f, 2.0f);
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 ambientIntensity = float3(1.0f, 1.0f, 1.0f);
	float3 cameraPos = App->GetCameraModule()->getPosition();
	glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, &cameraPos[0]);

	glUniform3fv(glGetUniformLocation(program, "lightDir"), 1, &lightDir[0]);
    glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, &lightColor[0]);
	glUniform3fv(glGetUniformLocation(program, "ambientIntensity"), 1, &ambientIntensity[0]);


    if (material != nullptr)
    {
        material->RenderMaterial(program);
    }


	if (indexCount > 0 && vao)
	{
		glBindVertexArray(vao);

		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}
	else if(vao) {
		glBindVertexArray(vao);

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}
}

