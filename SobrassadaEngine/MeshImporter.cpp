#include "MeshImporter.h"

#include "FileSystem.h"
#include "Globals.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"


namespace MeshImporter {

    bool ImportMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive, const std::string& filePath) {

        std::vector<Vertex> vertexBuffer;
        std::vector<unsigned int> indexBuffer;

        //assumes there's always position,normal and texcoord0 !
        const auto& itPos = primitive.attributes.find("POSITION"); //holds iterator position pointing for the POSITION key
        if (itPos != primitive.attributes.end())
        {
            const tinygltf::Accessor& posAcc = model.accessors[itPos->second]; //gives index of position accessor
            const tinygltf::BufferView& posView = model.bufferViews[posAcc.bufferView]; //defines byte offset, stride and buffer
            const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
            const unsigned char* bufferPos = &(posBuffer.data[posAcc.byteOffset + posView.byteOffset]); //position data begins here


            const auto& itNormal = primitive.attributes.find("NORMAL");
            if (itNormal != primitive.attributes.end()) {
                const tinygltf::Accessor& normAcc = model.accessors[itNormal->second];
                const tinygltf::BufferView& normView = model.bufferViews[normAcc.bufferView];
                const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];
                const unsigned char* bufferNormal = &(normBuffer.data[normAcc.byteOffset + normView.byteOffset]);


                const auto& itTexCoord = primitive.attributes.find("TEXCOORD_0"); //repeat for textures
                if (itTexCoord != primitive.attributes.end()) {
                    const tinygltf::Accessor& texAcc = model.accessors[itTexCoord->second];
                    const tinygltf::BufferView& texView = model.bufferViews[texAcc.bufferView];
                    const tinygltf::Buffer& texBuffer = model.buffers[texView.buffer];
                    const unsigned char* bufferTexCoord = &(texBuffer.data[texAcc.byteOffset + texView.byteOffset]);

                    // Combine positions, normals and texture coordinates - interleaved format

                    for (size_t i = 0; i < posAcc.count; ++i) {
                        Vertex vertex;

                        // Copy position data
                        memcpy(vertex.position, bufferPos, sizeof(float) * 3);

                        // Copy normal data
                        memcpy(vertex.normal, bufferNormal, sizeof(float) * 3);

                        // Copy texture coordinate data
                        memcpy(vertex.texCoord, bufferTexCoord, sizeof(float) * 2);

                        bufferPos += posView.byteStride;
                        bufferNormal += normView.byteStride;
                        bufferTexCoord += texView.byteStride;

                        vertexBuffer.push_back(vertex);

                    }
                }
            }
        }

        const auto& itIndices = primitive.indices;
        if (itIndices != -1) {
            const tinygltf::Accessor& indexAcc = model.accessors[itIndices];
            const tinygltf::BufferView& indexView = model.bufferViews[indexAcc.bufferView];
            const tinygltf::Buffer& indexBufferData = model.buffers[indexView.buffer];
            const unsigned char* bufferIndices = &(indexBufferData.data[indexAcc.byteOffset + indexView.byteOffset]);


            if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    unsigned char index;
                    memcpy(&index, bufferIndices, sizeof(unsigned char));
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    unsigned short index;
                    memcpy(&index, bufferIndices, sizeof(unsigned short));
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                for (size_t i = 0; i < indexAcc.count; ++i) {
                    unsigned int index;
                    memcpy(&index, bufferIndices, sizeof(unsigned int));
                    indexBuffer.push_back(index);
                    bufferIndices += indexView.byteStride;
                }
            }
        }

        // Extract material
        // Extract mode (0:points  1:lines  2:line loop  3:line strip  4:triangles)
        //check for no mateiral and default to triangle mode
        int materialIndex = (primitive.material != -1) ? primitive.material : 0;
        int mode = (primitive.mode != -1) ? primitive.mode : 4;

        // save to binary file.
        // 1 - NUMBER OF INDICES,  2 - NUMBER OF VERTICES  3 - MATERIAL  4 - DRAWING MODE
        unsigned int header[4] = {
          static_cast<unsigned int>(indexBuffer.size()),
          static_cast<unsigned int>(vertexBuffer.size()),
          static_cast<unsigned int>(materialIndex),
          static_cast<unsigned int>(mode)
        };

        unsigned int size = sizeof(header) + sizeof(Vertex) * vertexBuffer.size() + sizeof(unsigned int) * indexBuffer.size();

        char* fileBuffer = new char[size];
        char* cursor = fileBuffer;

        // header
        memcpy(cursor, header, sizeof(header));
        cursor += sizeof(header);

        // interleaved vertex data (position + normal + texCoord)
        memcpy(cursor, vertexBuffer.data(), sizeof(Vertex) * vertexBuffer.size());
        cursor += sizeof(Vertex) * vertexBuffer.size();

        // index data
        memcpy(cursor, indexBuffer.data(), sizeof(unsigned int) * indexBuffer.size());
        cursor += sizeof(unsigned int) * indexBuffer.size();

        //false = append
        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string savePath = "Library/Meshes/" + fileName + ".sobrassada";
        unsigned int bytesWritten = FileSystem::Save(savePath.c_str(), fileBuffer, size, false);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save mesh file: %s", savePath.c_str());
            return false;
        }

        GLOG("%s saved as binary", fileName.c_str());

        return true;
    }
};