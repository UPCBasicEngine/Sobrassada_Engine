#include "MeshImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace MeshImporter
{

    UID ImportMesh(
        const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive,
        const std::string& name, const char* filePath
    )
    {
        enum DataType dataType = UNSIGNED_CHAR;
        std::vector<Vertex> vertexBuffer;
        unsigned int indexMode = -1;
        std::vector<unsigned char> indexBufferChar;
        std::vector<unsigned short> indexBufferShort;
        std::vector<unsigned int> indexBufferInt;
        size_t posStride = 0, tanStride = 0, texStride = 0, normStride = 0;
        float3 minPos     = {0.0f, 0.0f, 0.0f};
        float3 maxPos     = {0.0f, 0.0f, 0.0f};

        const auto& itPos = primitive.attributes.find("POSITION");
        if (itPos != primitive.attributes.end())
        {
            const tinygltf::Accessor& posAcc    = model.accessors[itPos->second];
            const tinygltf::BufferView& posView = model.bufferViews[posAcc.bufferView];
            const tinygltf::Buffer& posBuffer   = model.buffers[posView.buffer];
            const unsigned char* bufferPos      = &(posBuffer.data[posAcc.byteOffset + posView.byteOffset]);
            posStride                           = posView.byteStride ? posView.byteStride : sizeof(float3);

            if (posAcc.minValues.size() == 3 && posAcc.maxValues.size() == 3)
            {
                minPos = float3(
                    static_cast<float>(posAcc.minValues[0]), static_cast<float>(posAcc.minValues[1]),
                    static_cast<float>(posAcc.minValues[2])
                );
                maxPos = float3(
                    static_cast<float>(posAcc.maxValues[0]), static_cast<float>(posAcc.maxValues[1]),
                    static_cast<float>(posAcc.maxValues[2])
                );
            }

            const auto& itNormal              = primitive.attributes.find("NORMAL");
            const unsigned char* bufferNormal = nullptr;
            if (itNormal != primitive.attributes.end())
            {
                const tinygltf::Accessor& normAcc    = model.accessors[itNormal->second];
                const tinygltf::BufferView& normView = model.bufferViews[normAcc.bufferView];
                const tinygltf::Buffer& normBuffer   = model.buffers[normView.buffer];
                bufferNormal                         = &(normBuffer.data[normAcc.byteOffset + normView.byteOffset]);
                normStride                           = normView.byteStride ? normView.byteStride : sizeof(float3);
            }

            const auto& itTexCoord              = primitive.attributes.find("TEXCOORD_0");
            const unsigned char* bufferTexCoord = nullptr;
            if (itTexCoord != primitive.attributes.end())
            {
                const tinygltf::Accessor& texAcc    = model.accessors[itTexCoord->second];
                const tinygltf::BufferView& texView = model.bufferViews[texAcc.bufferView];
                const tinygltf::Buffer& texBuffer   = model.buffers[texView.buffer];
                bufferTexCoord                      = &(texBuffer.data[texAcc.byteOffset + texView.byteOffset]);
                texStride                           = texView.byteStride ? texView.byteStride : sizeof(float2);
                // Combine positions, normals and texture coordinates - interleaved format
            }

            const auto& itTan              = primitive.attributes.find("TANGENT");
            const unsigned char* bufferTan = nullptr;
            if (itTan != primitive.attributes.end())
            {
                const tinygltf::Accessor& tanAcc    = model.accessors[itTan->second];
                const tinygltf::BufferView& tanView = model.bufferViews[tanAcc.bufferView];
                const tinygltf::Buffer& tanBuffer   = model.buffers[tanView.buffer];
                bufferTan                           = &(tanBuffer.data[tanAcc.byteOffset + tanView.byteOffset]);
                tanStride                           = tanView.byteStride ? tanView.byteStride : sizeof(float4);
            }

            vertexBuffer.reserve(posAcc.count);

            for (size_t i = 0; i < posAcc.count; ++i)
            {
                Vertex vertex;
                vertex.position  = *reinterpret_cast<const float3*>(bufferPos);
                vertex.tangent   = bufferTan ? *reinterpret_cast<const float4*>(bufferTan) : float4(0, 0, 0, 1);
                vertex.normal    = bufferNormal ? *reinterpret_cast<const float3*>(bufferNormal) : float3(0, 0, 0);
                vertex.texCoord  = bufferTexCoord ? *reinterpret_cast<const float2*>(bufferTexCoord) : float2(0, 0);

                bufferPos       += posStride;
                if (bufferNormal) bufferNormal += normStride;
                if (bufferTexCoord) bufferTexCoord += texStride;
                if (bufferTan) bufferTan += tanStride;

                vertexBuffer.push_back(vertex);
            }
        }

        const auto& itIndices = primitive.indices;
        if (itIndices != -1)
        {
            const tinygltf::Accessor& indexAcc      = model.accessors[itIndices];
            const tinygltf::BufferView& indexView   = model.bufferViews[indexAcc.bufferView];
            const tinygltf::Buffer& indexBufferData = model.buffers[indexView.buffer];
            const unsigned char* bufferIndices = &(indexBufferData.data[indexAcc.byteOffset + indexView.byteOffset]);
            size_t stride                      = (indexView.byteStride > 0) ? indexView.byteStride : 0;

            if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                indexBufferChar.reserve(indexAcc.count);
                stride   = (stride > 0) ? stride : sizeof(unsigned char);
                dataType = UNSIGNED_CHAR;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode = 0;
                    indexBufferChar.push_back(bufferIndices[i]);
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                indexBufferShort.reserve(indexAcc.count);
                stride   = (stride > 0) ? stride : sizeof(unsigned short);
                dataType = UNSIGNED_SHORT;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode            = 1;
                    unsigned short index = *reinterpret_cast<const unsigned short*>(bufferIndices + i * stride);
                    indexBufferShort.push_back(index);
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                indexBufferInt.reserve(indexAcc.count);
                stride   = (stride > 0) ? stride : sizeof(unsigned int);
                dataType = UNSIGNED_INT;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode          = 2;
                    unsigned int index = *reinterpret_cast<const unsigned int*>(bufferIndices + i * stride);
                    indexBufferInt.push_back(index);
                }
            }
        }

        // Extract material
        // Extract mode (0:points  1:lines  2:line loop  3:line strip  4:triangles)
        // check for no mateiral and default to triangle mode
        // int materialIndex      = (primitive.material != -1) ? primitive.material : 0;
        int mode               = (primitive.mode != -1) ? primitive.mode : 4;

        // save to binary file.
        // 1 - NUMBER OF INDICES,  2 - NUMBER OF VERTICES  3 - MATERIAL  4 - DRAWING MODE
        unsigned int header[4] = {0, 0, 0, 0};

        int indexBufferSize    = 0;

        if (dataType == DataType::UNSIGNED_CHAR)
        {
            indexBufferSize = sizeof(unsigned char) * (int)indexBufferChar.size();

            header[0]       = (unsigned int)(indexBufferChar.size());
            header[1]       = (unsigned int)(vertexBuffer.size());
            header[2]       = (unsigned int)(mode);
            header[3]       = (unsigned int)(indexMode);
        }
        if (dataType == DataType::UNSIGNED_SHORT)
        {
            indexBufferSize = sizeof(unsigned short) * (int)indexBufferShort.size();

            header[0]       = (unsigned int)(indexBufferShort.size());
            header[1]       = (unsigned int)(vertexBuffer.size());
            header[2]       = (unsigned int)(mode);
            header[3]       = (unsigned int)(indexMode);
        }
        if (dataType == DataType::UNSIGNED_INT)
        {
            indexBufferSize = sizeof(unsigned int) * (int)indexBufferInt.size();

            header[0]       = (unsigned int)(indexBufferInt.size());
            header[1]       = (unsigned int)(vertexBuffer.size());
            header[2]       = (unsigned int)(mode);
            header[3]       = (unsigned int)(indexMode);
        }

        unsigned int size = static_cast<unsigned int>(
            sizeof(header) + (sizeof(Vertex) * vertexBuffer.size()) + indexBufferSize + (sizeof(float3) * 2)
        );

        char* fileBuffer = new char[size];
        char* cursor     = fileBuffer;

        // header
        memcpy(cursor, header, sizeof(header));
        cursor += sizeof(header);

        // interleaved vertex data (position + tangent + normal + texCoord)
        memcpy(cursor, vertexBuffer.data(), sizeof(Vertex) * vertexBuffer.size());
        cursor += sizeof(Vertex) * vertexBuffer.size();

        // index data
        if (dataType == DataType::UNSIGNED_CHAR)
        {
            memcpy(cursor, indexBufferChar.data(), indexBufferSize);
        }
        if (dataType == DataType::UNSIGNED_SHORT)
        {
            memcpy(cursor, indexBufferShort.data(), indexBufferSize);
        }
        if (dataType == DataType::UNSIGNED_INT)
        {
            memcpy(cursor, indexBufferInt.data(), indexBufferSize);
        }
        cursor += indexBufferSize;

        memcpy(cursor, &minPos, sizeof(float3));
        cursor += sizeof(float3);
        memcpy(cursor, &maxPos, sizeof(float3));
        cursor                    += sizeof(float3);

        UID meshUID                = GenerateUID();

        std::string savePath       = MESHES_PATH + std::string("Mesh") + MESH_EXTENSION;
        UID finalMeshUID           = App->GetLibraryModule()->AssignFiletypeUID(meshUID, savePath);
        std::string fileName       = FileSystem::GetFileNameWithoutExtension(filePath);

        savePath                   = MESHES_PATH + std::to_string(finalMeshUID) + MESH_EXTENSION;

        unsigned int bytesWritten  = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, true);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save mesh file: %s", savePath.c_str());
            return 0;
        }

        // added mesh to meshes map
        App->GetLibraryModule()->AddMesh(finalMeshUID, name);
        App->GetLibraryModule()->AddResource(savePath, finalMeshUID);

        GLOG("%s saved as binary", fileName.c_str());

        return finalMeshUID;
    }

    ResourceMesh* LoadMesh(UID meshUID)
    {
        char* buffer          = nullptr;

        std::string path      = App->GetLibraryModule()->GetResourcePath(meshUID);

        unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load the .sobrassada file: ");
            return nullptr;
        }

        char* cursor = buffer;

        // Read header
        unsigned int header[4];
        // works
        memcpy(header, cursor, sizeof(header));

        cursor                   += sizeof(header);

        unsigned int indexCount   = header[0];
        unsigned int vertexCount  = header[1];
        // unsigned int materialIndex  = header[2];
        unsigned int mode         = header[2];
        unsigned int indexMode    = header[3];
        GLOG("The mode for the mesh is %d", mode);

        // Create Mesh
        std::vector<Vertex> tmpVertices;
        tmpVertices.reserve(vertexCount);

        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            Vertex vertex = *reinterpret_cast<Vertex*>(cursor);
            tmpVertices.push_back(vertex); // Add vertex to vector
            cursor += sizeof(Vertex);      // Move cursor forward
        }

        std::vector<unsigned int> tmpIndices;
        tmpIndices.reserve(indexCount);

        if (indexMode == 2) // unsigned int
        {
            const unsigned int* bufferInd = reinterpret_cast<const unsigned int*>(cursor);
            for (unsigned int i = 0; i < indexCount; ++i)
            {
                tmpIndices.push_back(bufferInd[i]);
            }
            cursor += sizeof(unsigned int) * indexCount;
        }
        else if (indexMode == 1) // unsigned short
        {
            const unsigned short* bufferInd = reinterpret_cast<const unsigned short*>(cursor);

            for (unsigned short i = 0; i < indexCount; ++i)
            {
                tmpIndices.push_back(bufferInd[i]);
            }
            cursor += sizeof(unsigned short) * indexCount;
        }
        else if (indexMode == 0) // unsigned byte
        {
            const unsigned char* bufferInd = reinterpret_cast<const unsigned char*>(cursor);
            for (unsigned char i = 0; i < indexCount; ++i)
            {
                tmpIndices.push_back(bufferInd[i]);
            }
            cursor += sizeof(unsigned char) * indexCount;
        }
        else
        {
            GLOG("No indices");
        }

        float3 minPos       = *reinterpret_cast<float3*>(cursor);
        cursor             += sizeof(float3);
        float3 maxPos       = *reinterpret_cast<float3*>(cursor);
        cursor             += sizeof(float3);

        ResourceMesh* mesh  = new ResourceMesh(meshUID, FileSystem::GetFileNameWithoutExtension(path), maxPos, minPos);

        mesh->LoadData(mode, tmpVertices, tmpIndices);

        delete[] buffer;

        // App->GetLibraryModule()->AddResource(savePath, finalMeshUID);

        return mesh;
    }
}; // namespace MeshImporter