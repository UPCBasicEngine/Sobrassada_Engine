#include "MeshImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "Mesh.h"
#include "MetaMesh.h"
#include "ProjectModule.h"
#include "ResourceMesh.h"

#include "Math/Quat.h"
#include "rapidjson/document.h"
#include "tiny_gltf.h"
#include <algorithm>
#include <vector>

namespace MeshImporter
{

    UID ImportMesh(
        const tinygltf::Model& model, const uint32_t meshIndex, const uint32_t primitiveIndex, const std::string& name,
        const float4x4& meshTransform, const char* sourceFilePath, const std::string& targetFilePath, UID sourceUID,
        UID defaultMatUID
    )
    {
        enum DataType indexType = UNSIGNED_CHAR;
        enum DataType jointType = UNSIGNED_CHAR;
        std::vector<Vertex> vertexBuffer;
        std::vector<unsigned char> indexBufferChar;
        std::vector<unsigned short> indexBufferShort;
        std::vector<unsigned int> indexBufferInt;
        size_t posStride = 0, tanStride = 0, texStride = 0, normStride = 0, jointStride = 0, weightsStride = 0;
        float3 minPos                        = {0.0f, 0.0f, 0.0f};
        float3 maxPos                        = {0.0f, 0.0f, 0.0f};
        bool generateTangents                = false;

        const tinygltf::Primitive& primitive = model.meshes[meshIndex].primitives[primitiveIndex];

        // First get the indices, the type is needed for the joints later
        const auto& itIndices                = primitive.indices;
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
                stride    = (stride > 0) ? stride : sizeof(unsigned char);
                indexType = UNSIGNED_CHAR;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    indexBufferChar.push_back(bufferIndices[i]);
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                indexBufferShort.reserve(indexAcc.count);
                stride    = (stride > 0) ? stride : sizeof(unsigned short);
                indexType = UNSIGNED_SHORT;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    unsigned short index = *reinterpret_cast<const unsigned short*>(bufferIndices + i * stride);
                    indexBufferShort.push_back(index);
                }
            }
            else if (indexAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                indexBufferInt.reserve(indexAcc.count);
                stride    = (stride > 0) ? stride : sizeof(unsigned int);
                indexType = UNSIGNED_INT;
                for (size_t i = 0; i < indexAcc.count; ++i)
                {
                    unsigned int index = *reinterpret_cast<const unsigned int*>(bufferIndices + i * stride);
                    indexBufferInt.push_back(index);
                }
            }
        }

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
                generateTangents                    = true;
            }
            else GLOG("Model without tangents");

            const auto& itJoints              = primitive.attributes.find("JOINTS_0");
            const unsigned char* bufferJoints = nullptr;
            if (itJoints != primitive.attributes.end())
            {
                //GLOG("GET JOINTS");
                const tinygltf::Accessor& jointAcc    = model.accessors[itJoints->second];
                const tinygltf::BufferView& jointView = model.bufferViews[jointAcc.bufferView];
                const tinygltf::Buffer& jointBuffer   = model.buffers[jointView.buffer];
                bufferJoints                          = &(jointBuffer.data[jointAcc.byteOffset + jointView.byteOffset]);

                switch (jointAcc.componentType)
                {
                case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE):
                    jointType   = UNSIGNED_CHAR;
                    jointStride = jointView.byteStride ? jointView.byteStride : sizeof(unsigned char) * 4;
                    break;
                case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT):
                    jointType   = UNSIGNED_SHORT;
                    jointStride = jointView.byteStride ? jointView.byteStride : sizeof(unsigned short) * 4;
                    break;
                }
            }

            const auto& itWeights              = primitive.attributes.find("WEIGHTS_0");
            const unsigned char* bufferWeights = nullptr;
            if (itWeights != primitive.attributes.end())
            {
                //GLOG("GET WEIGHTS");
                const tinygltf::Accessor& weightsAcc    = model.accessors[itWeights->second];
                const tinygltf::BufferView& weightsView = model.bufferViews[weightsAcc.bufferView];
                const tinygltf::Buffer& weightsBuffer   = model.buffers[weightsView.buffer];
                bufferWeights = &(weightsBuffer.data[weightsAcc.byteOffset + weightsView.byteOffset]);
                weightsStride = weightsView.byteStride ? weightsView.byteStride : sizeof(float4);
            }

            vertexBuffer.reserve(posAcc.count);

            for (size_t i = 0; i < posAcc.count; ++i)
            {
                Vertex vertex;
                vertex.position   = *reinterpret_cast<const float3*>(bufferPos);
                vertex.tangent    = bufferTan ? *reinterpret_cast<const float4*>(bufferTan) : float4(0, 0, 0, 1);
                vertex.normal     = bufferNormal ? *reinterpret_cast<const float3*>(bufferNormal) : float3(0, 0, 0);
                vertex.texCoord   = bufferTexCoord ? *reinterpret_cast<const float2*>(bufferTexCoord) : float2(0, 0);

                // Check all weights don't add up to more than 1
                float totalWeight = 0;
                for (int i = 0; i < 4; ++i)
                {
                    totalWeight += vertex.weights[i];
                }
                // If not, renormalize them
                if (totalWeight > 1)
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        vertex.weights[i] /= totalWeight;
                    }
                }

                if (bufferJoints)
                {
                    switch (jointType)
                    {
                    case (UNSIGNED_CHAR):
                    {
                        const unsigned char* joints = reinterpret_cast<const unsigned char*>(bufferJoints);
                        vertex.joint[0]             = (unsigned int)joints[0];
                        vertex.joint[1]             = (unsigned int)joints[1];
                        vertex.joint[2]             = (unsigned int)joints[2];
                        vertex.joint[3]             = (unsigned int)joints[3];
                        break;
                    }
                    case (UNSIGNED_SHORT):
                    {
                        const unsigned short* joints = reinterpret_cast<const unsigned short*>(bufferJoints);
                        vertex.joint[0]              = (unsigned int)joints[0];
                        vertex.joint[1]              = (unsigned int)joints[1];
                        vertex.joint[2]              = (unsigned int)joints[2];
                        vertex.joint[3]              = (unsigned int)joints[3];
                        break;
                    }
                    default:
                        GLOG("Error with jointType");
                        break;
                    }
                }
                else
                {
                    vertex.joint[0] = 0;
                    vertex.joint[1] = 0;
                    vertex.joint[2] = 0;
                    vertex.joint[3] = 1;
                }

                vertex.weights  = bufferWeights ? *reinterpret_cast<const float4*>(bufferWeights) : float4(0, 0, 0, 1);

                bufferPos      += posStride;
                if (bufferTan) bufferTan += tanStride;
                if (bufferNormal) bufferNormal += normStride;
                if (bufferTexCoord) bufferTexCoord += texStride;

                if (bufferJoints) bufferJoints += jointStride;
                if (bufferWeights) bufferWeights += weightsStride;

                vertexBuffer.push_back(vertex);
            }
        }

        // Extract mode (0:points  1:lines  2:line loop  3:line strip  4:triangles)
        int mode               = (primitive.mode != -1) ? primitive.mode : 4;

        // save to binary file.
        // 1 - NUMBER OF INDICES,  2 - NUMBER OF VERTICES  3 - MODE  4 - INDEX MODE
        unsigned int header[4] = {0, 0, 0, 0};

        int indexBufferSize    = 0;

        switch (indexType)
        {
        case (UNSIGNED_CHAR):
            indexBufferSize = sizeof(unsigned char) * (int)indexBufferChar.size();
            header[0]       = (unsigned int)(indexBufferChar.size());
            break;
        case (UNSIGNED_SHORT):
            indexBufferSize = sizeof(unsigned short) * (int)indexBufferShort.size();
            header[0]       = (unsigned int)(indexBufferShort.size());
            break;
        case (UNSIGNED_INT):
            indexBufferSize = sizeof(unsigned int) * (int)indexBufferInt.size();
            header[0]       = (unsigned int)(indexBufferInt.size());
            break;
        default:
            GLOG("Error with indexType");
            break;
        }

        header[1]         = (unsigned int)(vertexBuffer.size());
        header[2]         = (unsigned int)(mode);
        header[3]         = static_cast<unsigned int>(indexType);

        unsigned int size = static_cast<unsigned int>(
            sizeof(header) + (sizeof(Vertex) * vertexBuffer.size()) + indexBufferSize + (sizeof(float3) * 2)
        );

        char* fileBuffer = new char[size];
        char* cursor     = fileBuffer;

        // header
        memcpy(cursor, header, sizeof(header));
        cursor += sizeof(header);

        // order matters:
        // interleaved vertex data (position + tangent + normal + texCoord + joints + weights)
        memcpy(cursor, vertexBuffer.data(), sizeof(Vertex) * vertexBuffer.size());
        cursor += sizeof(Vertex) * vertexBuffer.size();

        // index data
        if (indexType == DataType::UNSIGNED_CHAR)
        {
            memcpy(cursor, indexBufferChar.data(), indexBufferSize);
        }
        if (indexType == DataType::UNSIGNED_SHORT)
        {
            memcpy(cursor, indexBufferShort.data(), indexBufferSize);
        }
        if (indexType == DataType::UNSIGNED_INT)
        {
            memcpy(cursor, indexBufferInt.data(), indexBufferSize);
        }
        cursor += indexBufferSize;

        memcpy(cursor, &minPos, sizeof(float3));
        cursor += sizeof(float3);
        memcpy(cursor, &maxPos, sizeof(float3));
        cursor += sizeof(float3);

        UID finalMeshUID;
        if (sourceUID == INVALID_UID)
        {
            // Importing mesh from gltf drag n drop
            UID meshUID                = GenerateUID();
            meshUID                    = App->GetLibraryModule()->AssignFiletypeUID(meshUID, FileType::Mesh);

            std::string assetPath      = ASSETS_PATH + FileSystem::GetFileNameWithExtension(sourceFilePath);

            UID prefix                 = meshUID / UID_PREFIX_DIVISOR;
            const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                         std::to_string(prefix) + FILENAME_SEPARATOR + name + META_EXTENSION;
            finalMeshUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
            if (finalMeshUID == INVALID_UID) finalMeshUID = meshUID;

            MetaMesh meta(
                finalMeshUID, assetPath, generateTangents, meshTransform, defaultMatUID, meshIndex, primitiveIndex
            );
            meta.Save(name, assetPath);
        }
        else finalMeshUID = sourceUID; // Importing mesh for meta file

        std::string saveFilePath  = targetFilePath + MESHES_PATH + std::to_string(finalMeshUID) + MESH_EXTENSION;
        unsigned int bytesWritten = (unsigned int)FileSystem::Save(saveFilePath.c_str(), fileBuffer, size, true);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save mesh file: %s", saveFilePath.c_str());
            return 0;
        }

        // TODO Reload mesh resource so changes are viewable immediately in the scene

        // added mesh to meshes map
        App->GetLibraryModule()->AddMesh(finalMeshUID, name);
        App->GetLibraryModule()->AddName(name, finalMeshUID);
        App->GetLibraryModule()->AddResource(saveFilePath, finalMeshUID);

        //GLOG("%s saved as binary", name.c_str());

        return finalMeshUID;
    }

    const float4x4 GetMeshDefaultTransform(const tinygltf::Model& model, const std::string& name)
    {
        const std::vector<tinygltf::Node>& nodes = model.nodes;

        const auto& it                           = std::find_if(
            nodes.begin(), nodes.end(),
            [&name](const tinygltf::Node& node) { return node.mesh != -1 && node.name == name; }
        );

        if (it == nodes.end())
        {
            return float4x4::identity;
        }

        const tinygltf::Node& node = *it;

        return GetNodeTransform(node);
    }

    const float4x4 GetNodeTransform(const tinygltf::Node& node)
    {
        if (!node.matrix.empty())
        {
            // glTF stores matrices in COLUMN-MAJOR order, same as MathGeoLib
            float4x4 matrix = float4x4(
                (float)node.matrix[0], (float)node.matrix[1], (float)node.matrix[2], (float)node.matrix[3],
                (float)node.matrix[4], (float)node.matrix[5], (float)node.matrix[6], (float)node.matrix[7],
                (float)node.matrix[8], (float)node.matrix[9], (float)node.matrix[10], (float)node.matrix[11],
                (float)node.matrix[12], (float)node.matrix[13], (float)node.matrix[14], (float)node.matrix[15]
            );
            return matrix.Transposed(); // Probably need the Transposed(), but has not been tested
        }

        // Default values
        float3 translation = float3::zero;
        Quat rotation      = Quat::identity;
        float3 scale       = float3::one;

        if (!node.translation.empty())
            translation = float3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);

        if (!node.rotation.empty())
            rotation = Quat(
                (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]
            ); // glTF stores as [x, y, z, w]

        if (!node.scale.empty()) scale = float3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);

        float4x4 matrix = float4x4::FromTRS(translation, rotation, scale);
        return matrix;
    }

    ResourceMesh* LoadMesh(UID meshUID)
    {
        char* buffer          = nullptr;

        std::string path      = App->GetLibraryModule()->GetResourcePath(meshUID);
        std::string name      = App->GetLibraryModule()->GetResourceName(meshUID);

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
        unsigned int mode         = header[2];
        unsigned int indexMode    = header[3];
        // GLOG("The mode for the mesh is %d", mode);

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

        if (indexMode != -1)
        {
            tmpIndices.reserve(indexCount);

            if (indexMode == 0) // unsigned byte
            {
                const unsigned char* bufferInd = reinterpret_cast<const unsigned char*>(cursor);
                for (unsigned int i = 0; i < indexCount; ++i)
                {
                    tmpIndices.push_back(bufferInd[i]);
                }
                cursor += sizeof(unsigned char) * indexCount;
            }
            else if (indexMode == 1) // unsigned short
            {
                const unsigned short* bufferInd = reinterpret_cast<const unsigned short*>(cursor);

                for (unsigned int i = 0; i < indexCount; ++i)
                {
                    tmpIndices.push_back(bufferInd[i]);
                }
                cursor += sizeof(unsigned short) * indexCount;
            }
            else if (indexMode == 2) // unsigned int
            {
                const unsigned int* bufferInd = reinterpret_cast<const unsigned int*>(cursor);
                for (unsigned int i = 0; i < indexCount; ++i)
                {
                    tmpIndices.push_back(bufferInd[i]);
                }
                cursor += sizeof(unsigned int) * indexCount;
            }
        }
        else
        {
            GLOG("No indices");
        }

        float3 minPos  = *reinterpret_cast<float3*>(cursor);
        cursor        += sizeof(float3);
        float3 maxPos  = *reinterpret_cast<float3*>(cursor);
        cursor        += sizeof(float3);

        rapidjson::Value importOptions;
        App->GetLibraryModule()->GetImportOptions(meshUID, importOptions);

        ResourceMesh* mesh = new ResourceMesh(meshUID, name, maxPos, minPos, importOptions);

        mesh->LoadData(mode, tmpVertices, tmpIndices);

        delete[] buffer;

        return mesh;
    }
}; // namespace MeshImporter