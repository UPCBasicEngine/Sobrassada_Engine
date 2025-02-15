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

        std::vector<Vertex> vertexBuffer;
        std::vector<unsigned int> indexBuffer;
        size_t texStride       = 0;
        size_t tanStride       = 0;
        size_t posStride       = 0;
        size_t normStride      = 0;
        unsigned int indexMode = -1;
        const auto& itPos =
            primitive.attributes.find("POSITION"); // holds iterator position pointing for the POSITION key
        if (itPos != primitive.attributes.end())
        {
            const tinygltf::Accessor& posAcc = model.accessors[itPos->second]; // gives index of position accessor
            const tinygltf::BufferView& posView =
                model.bufferViews[posAcc.bufferView]; // defines byte offset, stride and buffer
            const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
            const unsigned char* bufferPos =
                &(posBuffer.data[posAcc.byteOffset + posView.byteOffset]); // position data begins here
            posStride            = posView.byteStride ? posView.byteStride : sizeof(float) * 3;

            const auto& itNormal = primitive.attributes.find("NORMAL");
            if (itNormal != primitive.attributes.end())
            {
                const tinygltf::Accessor& normAcc    = model.accessors[itNormal->second];
                const tinygltf::BufferView& normView = model.bufferViews[normAcc.bufferView];
                const tinygltf::Buffer& normBuffer   = model.buffers[normView.buffer];
                const unsigned char* bufferNormal    = &(normBuffer.data[normAcc.byteOffset + normView.byteOffset]);
                normStride                           = normView.byteStride ? normView.byteStride : sizeof(float) * 3;

                const auto& itTexCoord               = primitive.attributes.find("TEXCOORD_0"); // repeat for textures
                size_t texStride                     = 0;
                if (itTexCoord != primitive.attributes.end())
                {

                    const tinygltf::Accessor& texAcc    = model.accessors[itTexCoord->second];
                    const tinygltf::BufferView& texView = model.bufferViews[texAcc.bufferView];
                    const tinygltf::Buffer& texBuffer   = model.buffers[texView.buffer];
                    const unsigned char* bufferTexCoord = &(texBuffer.data[texAcc.byteOffset + texView.byteOffset]);
                    texStride                           = texView.byteStride ? texView.byteStride : sizeof(float) * 2;
                    // Combine positions, normals and texture coordinates - interleaved format

                    const auto& itTan                   = primitive.attributes.find("TANGENT");
                    if (itTan != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& tanAcc    = model.accessors[itTan->second];
                        const tinygltf::BufferView& tanView = model.bufferViews[tanAcc.bufferView];
                        const tinygltf::Buffer& tanBuffer   = model.buffers[tanView.buffer];
                        const unsigned char* bufferTan      = &(tanBuffer.data[tanAcc.byteOffset + tanView.byteOffset]);
                        tanStride = tanView.byteStride ? tanView.byteStride : sizeof(float) * 4;

                        for (size_t i = 0; i < posAcc.count; ++i)
                        {
                            Vertex vertex;

                            // Copy position data by dereferencing pointer casted to float3
                            vertex.position  = *reinterpret_cast<const float3*>(bufferPos);

                            vertex.tangent   = *reinterpret_cast<const float4*>(bufferTan);

                            // Copy normal data
                            vertex.normal    = *reinterpret_cast<const float3*>(bufferNormal);

                            // Copy texture coordinate data
                            vertex.texCoord  = *reinterpret_cast<const float2*>(bufferTexCoord);

                            bufferPos       += posView.byteStride;
                            bufferTan       += tanView.byteStride;
                            bufferNormal    += normView.byteStride;
                            bufferTexCoord  += texView.byteStride;

                            vertexBuffer.push_back(vertex);
                        }
                    }
                    else
                    {
                        // possible crash here
                        for (size_t i = 0; i < posAcc.count; ++i)
                        {
                            Vertex vertex;

                            // Copy position data by dereferencing pointer casted to float3
                            vertex.position  = *reinterpret_cast<const float3*>(bufferPos);

                            vertex.tangent   = float4(0, 0, 0, 1); // A neutral tangent
                            // Copy normal data
                            vertex.normal    = *reinterpret_cast<const float3*>(bufferNormal);

                            // Copy texture coordinate data
                            vertex.texCoord  = *reinterpret_cast<const float2*>(bufferTexCoord);

                            bufferPos       += posView.byteStride;
                            bufferNormal    += normView.byteStride;
                            bufferTexCoord  += texView.byteStride;

                            vertexBuffer.push_back(vertex);
                        }
                    }
                }
            }
        }

        const auto& itIndices = primitive.indices;
        if (itIndices != -1)
        {
            const tinygltf::Accessor& indexAcc      = model.accessors[itIndices];
            const tinygltf::BufferView& indexView   = model.bufferViews[indexAcc.bufferView];
            const tinygltf::Buffer& indexBufferData = model.buffers[indexView.buffer];
            const unsigned char* bufferIndices = &(indexBufferData.data[indexAcc.byteOffset + indexView.byteOffset]);

            if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode = 0;
                    unsigned char index;
                    index = *reinterpret_cast<const unsigned char*>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode = 1;
                    unsigned short index;
                    index = *reinterpret_cast<const unsigned short*>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexMode = 2;
                    unsigned int index;
                    index = *reinterpret_cast<const unsigned int*>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
        }

        // Extract material
        // Extract mode (0:points  1:lines  2:line loop  3:line strip  4:triangles)
        // check for no mateiral and default to triangle mode
        //
        // int materialIndex      = (primitive.material != -1) ? primitive.material : 0;
        int mode               = (primitive.mode != -1) ? primitive.mode : 4;

        // save to binary file.
        //
        //

        // 1 - NUMBER OF INDICES,  2 - NUMBER OF VERTICES  3 - MATERIAL  4 - DRAWING MODE
        unsigned int header[4] = {
            static_cast<unsigned int>(indexBuffer.size()), static_cast<unsigned int>(vertexBuffer.size()),
            static_cast<unsigned int>(mode), static_cast<unsigned int>(indexMode)
        };

        unsigned int size = (unsigned int)(sizeof(header) + sizeof(Vertex) * vertexBuffer.size() +
                                           sizeof(unsigned int) * indexBuffer.size());

        char* fileBuffer  = new char[size];
        char* cursor      = fileBuffer;

        // header
        memcpy(cursor, header, sizeof(header));
        cursor += sizeof(header);

        // interleaved vertex data (position + tangent + normal + texCoord)
        memcpy(cursor, vertexBuffer.data(), sizeof(Vertex) * vertexBuffer.size());
        cursor += sizeof(Vertex) * vertexBuffer.size();

        // index data
        memcpy(cursor, indexBuffer.data(), sizeof(unsigned int) * indexBuffer.size());
        cursor                    += sizeof(unsigned int) * indexBuffer.size();

        UID meshUID                = GenerateUID();

        // false = append
        std::string fileName       = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string savePath       = MESHES_PATH + name + MESH_EXTENSION;
        unsigned int bytesWritten  = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, false);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save mesh file: %s", savePath.c_str());
            return 0;
        }

        UID finalMeshUID = App->GetLibraryModule()->AssignFiletypeUID(meshUID, savePath);

        // added mesh to meshes map
        App->GetLibraryModule()->AddMesh(finalMeshUID, name);
        App->GetLibraryModule()->AddResource(savePath, finalMeshUID);

        GLOG("%s saved as binary", fileName.c_str());

        return meshUID;
    }

    ResourceMesh* LoadMesh(UID meshUID)
    {

        char* buffer          = nullptr;

        std::string path      = App->GetLibraryModule()->GetResourcePath(meshUID);

        unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load the .sobrassada file: ");
            return nullptr; // Return 0 or an appropriate error code
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

        /*
        std::move(
            reinterpret_cast<Vertex *>(cursor), reinterpret_cast<Vertex *>(cursor) + vertexCount, tmpVertices.begin()
        );
        */

        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            Vertex vertex = *reinterpret_cast<Vertex*>(cursor);
            tmpVertices.push_back(vertex); // Add vertex to vector
            cursor += sizeof(Vertex);      // Move cursor forward
        }

        // mesh->SetVertices(std::move(tmpVertices));

        std::vector<unsigned int> tmpIndices;
        tmpIndices.reserve(indexCount);

        // Read index buffer
        /*
        std::move(
            reinterpret_cast<unsigned int*>(cursor), reinterpret_cast<unsigned int*>(cursor) + indexCount,
            tmpIndices.begin()
        );
        */
        if (indexMode == 2) // unsigned int
        {
            const uint32_t* bufferInd = reinterpret_cast<const uint32_t*>(buffer);
            for (uint32_t i = 0; i < indexCount; ++i)
                tmpIndices.push_back(i);
        }
        else if (indexMode == 1) // unsigned short
        {
            const uint16_t* bufferInd = reinterpret_cast<const uint16_t*>(buffer);
            for (uint16_t i = 0; i < indexCount; ++i)
                tmpIndices.push_back(i);
        }
        else if (indexMode == 0) // unsigned byte
        {
            const uint8_t* bufferInd = reinterpret_cast<const uint8_t*>(buffer);
            for (uint8_t i = 0; i < indexCount; ++i)
                tmpIndices.push_back(i);
        }
        else
        {
            GLOG("No indices");
        }

        /*
        for (unsigned int i = 0; i < indexCount; ++i)
        {
            unsigned int index = *reinterpret_cast<unsigned unsigned char*>(cursor);
            tmpIndices.push_back(index);    // Add vertex to vector
            cursor += sizeof(unsigned int); // Move cursor forward
        }
        */

        ResourceMesh* mesh = new ResourceMesh(meshUID, FileSystem::GetFileNameWithoutExtension(path));

        mesh->LoadData(mode, tmpVertices, tmpIndices);

        delete[] buffer;

        return mesh;
    }

}; // namespace MeshImporter