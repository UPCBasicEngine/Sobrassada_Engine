#include "MeshImporter.h"

#include "FileSystem.h"


#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace MeshImporter
{

    UID ImportMesh(
        const tinygltf::Model &model, const tinygltf::Mesh &mesh, const tinygltf::Primitive &primitive,
        const std::string &name, const char * filePath
    )
    {

        std::vector<Vertex> vertexBuffer;
        std::vector<unsigned int> indexBuffer;

        const auto &itPos =
            primitive.attributes.find("POSITION"); // holds iterator position pointing for the POSITION key
        if (itPos != primitive.attributes.end())
        {
            const tinygltf::Accessor &posAcc = model.accessors[itPos->second]; // gives index of position accessor
            const tinygltf::BufferView &posView = model.bufferViews[posAcc.bufferView]; // defines byte offset, stride and buffer
            const tinygltf::Buffer &posBuffer = model.buffers[posView.buffer];
            const unsigned char *bufferPos =
                &(posBuffer.data[posAcc.byteOffset + posView.byteOffset]); // position data begins here

            const auto &itTan =
                primitive.attributes.find("TANGENT"); if (itTan != primitive.attributes.end())
            {
                const tinygltf::Accessor &tanAcc = model.accessors[itTan->second];
                const tinygltf::BufferView &tanView = model.bufferViews[tanAcc.bufferView];
                const tinygltf::Buffer &tanBuffer = model.buffers[tanView.buffer];
                const unsigned char *bufferTan =
                    &(tanBuffer.data[tanAcc.byteOffset + tanView.byteOffset]); 

                const auto &itNormal = primitive.attributes.find("NORMAL");
                if (itNormal != primitive.attributes.end())
                {
                    const tinygltf::Accessor &normAcc    = model.accessors[itNormal->second];
                    const tinygltf::BufferView &normView = model.bufferViews[normAcc.bufferView];
                    const tinygltf::Buffer &normBuffer   = model.buffers[normView.buffer];
                    const unsigned char *bufferNormal    = &(normBuffer.data[normAcc.byteOffset + normView.byteOffset]);

                    const auto &itTexCoord = primitive.attributes.find("TEXCOORD_0"); // repeat for textures
                    if (itTexCoord != primitive.attributes.end())
                    {
                        const tinygltf::Accessor &texAcc    = model.accessors[itTexCoord->second];
                        const tinygltf::BufferView &texView = model.bufferViews[texAcc.bufferView];
                        const tinygltf::Buffer &texBuffer   = model.buffers[texView.buffer];
                        const unsigned char *bufferTexCoord = &(texBuffer.data[texAcc.byteOffset + texView.byteOffset]);

                        // Combine positions, normals and texture coordinates - interleaved format

                        for (size_t i = 0; i < posAcc.count; ++i)
                        {
                            Vertex vertex;

                            // Copy position data by dereferencing pointer casted to float3
                            vertex.position  = *reinterpret_cast<const float3 *>(bufferPos);

                            vertex.tangent = *reinterpret_cast<const float4 *>(bufferTan);

                            // Copy normal data
                            vertex.normal    = *reinterpret_cast<const float3 *>(bufferNormal);

                            // Copy texture coordinate data
                            vertex.texCoord  = *reinterpret_cast<const float2 *>(bufferTexCoord);

                            bufferPos       += posView.byteStride;
                            bufferTan       += tanView.byteStride;
                            bufferNormal    += normView.byteStride;
                            bufferTexCoord  += texView.byteStride;

                            vertexBuffer.push_back(vertex);
                        }
                    }
                }
            }
        }

        const auto &itIndices = primitive.indices;
        if (itIndices != -1)
        {
            const tinygltf::Accessor &indexAcc      = model.accessors[itIndices];
            const tinygltf::BufferView &indexView   = model.bufferViews[indexAcc.bufferView];
            const tinygltf::Buffer &indexBufferData = model.buffers[indexView.buffer];
            const unsigned char *bufferIndices = &(indexBufferData.data[indexAcc.byteOffset + indexView.byteOffset]);


            if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    unsigned char index;
                    index = *reinterpret_cast<const unsigned char *>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    unsigned short index;
                    index = *reinterpret_cast<const unsigned short *>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    unsigned int index;
                    index = *reinterpret_cast<const unsigned int *>(bufferIndices);
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
        }

        // Extract material
        // Extract mode (0:points  1:lines  2:line loop  3:line strip  4:triangles)
        // check for no mateiral and default to triangle mode
        int materialIndex      = (primitive.material != -1) ? primitive.material : 0;
        int mode               = (primitive.mode != -1) ? primitive.mode : 4;




        // save to binary file.
        // 1 - NUMBER OF INDICES,  2 - NUMBER OF VERTICES  3 - MATERIAL  4 - DRAWING MODE
        unsigned int header[4] = {
            static_cast<unsigned int>(indexBuffer.size()), static_cast<unsigned int>(vertexBuffer.size()),
            static_cast<unsigned int>(materialIndex), static_cast<unsigned int>(mode)
        };

        unsigned int size =
            (unsigned int)(sizeof(header) + sizeof(Vertex) * vertexBuffer.size() + sizeof(unsigned int) * indexBuffer.size());

        char *fileBuffer = new char[size];
        char *cursor     = fileBuffer;

        // header
        memcpy(cursor, header, sizeof(header));
        cursor += sizeof(header);

        // interleaved vertex data (position + normal + texCoord)
        memcpy(cursor, vertexBuffer.data(), sizeof(Vertex) * vertexBuffer.size());
        cursor += sizeof(Vertex) * vertexBuffer.size();

        // index data
        memcpy(cursor, indexBuffer.data(), sizeof(unsigned int) * indexBuffer.size());
        cursor                    += sizeof(unsigned int) * indexBuffer.size();

        UID meshUID               = GenerateUID();
        // false = append
        std::string fileName       = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string savePath       = MESHES_PATH + std::to_string(meshUID) + FILE_EXTENSION;
        unsigned int bytesWritten  = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, false);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save mesh file: %s", savePath.c_str());
            return 0;
        }

        GLOG("%s saved as binary", fileName.c_str());

        return meshUID;
    }

    std::shared_ptr<Mesh> LoadMesh(const char *path)
    {
        char *buffer = nullptr;
        char *cursor = buffer;

        unsigned int fileSize = FileSystem::Load(path, &buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load the .sobrassada file: ");
            return nullptr; // Return 0 or an appropriate error code
        }

        // Read header
        unsigned int header[4];
        memcpy(header, cursor, sizeof(header));
        cursor += sizeof(header);

        unsigned int indexCount     = header[0];
        unsigned int vertexCount    = header[1];
        unsigned int materialIndex  = header[2];
        unsigned int mode           = header[3];

        // Create Mesh

        auto mesh                   = std::make_unique<Mesh>();
        mesh->SetMaterialIndex(materialIndex);
        mesh->SetMode(mode);

        std::vector<Vertex> tmpVertices;
        tmpVertices.reserve(vertexCount);

        std::move(
            reinterpret_cast<Vertex *>(cursor), reinterpret_cast<Vertex *>(cursor) + vertexCount, tmpVertices.begin()
        );

        mesh->SetVertices(std::move(tmpVertices));
        cursor += vertexCount * sizeof(Vertex);

        std::vector<unsigned int> tmpIndices;
        tmpIndices.reserve(indexCount);

        // Read index buffer
        std::move(
            reinterpret_cast<unsigned int *>(cursor), reinterpret_cast<unsigned int *>(cursor) + indexCount,
            tmpIndices.begin()
        );
        mesh->SetIndices(std::move(tmpIndices));

        delete[] buffer;

        return mesh;
    }

};