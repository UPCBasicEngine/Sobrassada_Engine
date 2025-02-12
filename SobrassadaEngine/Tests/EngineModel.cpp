#include "EngineModel.h"
#include "Globals.h"
#include "EngineMesh.h"
#include "MathGeoLib.h"
#include "Components/ComponentMaterial.h"
#include "Application.h"
#include "TextureModuleTest.h"
#include "glew.h"
#include "DirectXTex/DirectXTex.h"
#include <string>
#include <unordered_set>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#include "tiny_gltf.h"

EngineModel::EngineModel()
{
	minValues = float3::zero;
	maxValues = float3::zero;
}

EngineModel::~EngineModel()
{
	ClearVectors();
}

void EngineModel::Load(const char* modelPath)
{
	ClearVectors();

	GLOG("Loading model: %s", modelPath);

	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;

	bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, modelPath);

	if (!loadOk)
	{
		GLOG("Error loading model %s: %s", modelPath, error.c_str());
	}

	// Checking for root node in nodes to start the model loading, if no default scene error in model doc.
	int rootPosition = model.scenes[model.defaultScene].nodes.size() > 0 ? model.scenes[model.defaultScene].nodes[0] : -1;
	if (rootPosition < 0) return;

	float4x4 basicModelMatrix = float4x4::identity;
	LoadRecursive(model, basicModelMatrix, rootPosition);

	LoadMaterials(model, modelPath);
}

void EngineModel::LoadMaterials(const tinygltf::Model& sourceModel, const char* modelPath)
{
    materials.clear();
    int id = 0;

    for (const auto &srcMaterial : sourceModel.materials)
    {
        ComponentMaterial *material = new ComponentMaterial;
        material->LoadMaterial(srcMaterial, sourceModel, modelPath);
        
        materials.push_back(material);
        id++;
    }
}

void EngineModel::Render(int program, unsigned int cameraUBO)
{
	for (EngineMesh* currentMesh : meshes)
	{
        std::vector<int>& indices = currentMesh->GetMaterialIndices();
		for (size_t i = 0; i < indices.size(); ++i)
		{
            ComponentMaterial* material = &GetMaterial(indices[i]);
			int texturePostiion = textures.size() > 0 ? renderTexture > -1 ? textures[renderTexture] : textures[textures.size() - 1] : 0;
            //currentMesh->Render(program, material, cameraUBO);
        }
	}
}

void EngineModel::LoadRecursive(const tinygltf::Model& sourceModel, const float4x4& parentModelMatrix, int currentNodePosition)
{
	const tinygltf::Node currentNode = sourceModel.nodes[currentNodePosition];

	// Creating basic model transform matrix if matrix data exists
	float4x4 modelMatrix = float4x4::identity;

	if (currentNode.matrix.size() > 0)
	{
		for (int i = 0; i < currentNode.matrix.size(); ++i)
		{
			modelMatrix[i / 4][i % 4] = (float)currentNode.matrix[i];
		}
	}
	else
	{
		// Creating basic model transform matrix based on tranlation, rotation and scale information
		Quat finalRotation;
		float3 translation, scale;

		translation = float3::zero;
		scale = float3::one;
		finalRotation = Quat(0, 0, 0, 1);

		if (currentNode.rotation.size() > 0)
		{
			finalRotation = Quat((float)currentNode.rotation[0], (float)currentNode.rotation[1], (float)currentNode.rotation[2], (float)currentNode.rotation[3]);
		}

		if (currentNode.translation.size() > 0)
		{
			translation = float3((float)currentNode.translation[0], (float)currentNode.translation[1], (float)currentNode.translation[2]);
		}

		if (currentNode.scale.size() > 0)
		{
			scale = float3((float)currentNode.scale[0], (float)currentNode.scale[1], (float)currentNode.scale[2]);
		}

		modelMatrix = float4x4::FromTRS(
			translation,
			finalRotation,
			scale
		);
	}

	// Apply parentModelMatrix to current node one
	modelMatrix = parentModelMatrix * modelMatrix;

	// If this node contains meshes then load them and apply the parents model matrix
	if (currentNode.mesh >= 0)
	{
		for (const tinygltf::Primitive& primitive : sourceModel.meshes[currentNode.mesh].primitives)
		{
			EngineMesh* newMesh = new EngineMesh();
			newMesh->LoadVBO(sourceModel, sourceModel.meshes[currentNode.mesh], primitive);
			if (primitive.indices >= 0) newMesh->LoadEBO(sourceModel, sourceModel.meshes[currentNode.mesh], primitive);
			newMesh->CreateVAO();

			/*float3 meshMaxValues = modelMatrix.MulPos(newMesh->GetMaximumPosition());
			float3 meshMinValues = modelMatrix.MulPos(newMesh->GetMinimumPosition());

			if (firstMesh)
			{
				firstMesh = false;
				maxValues = meshMaxValues;
				minValues = meshMinValues;
			}
			else
			{
				maxValues = float3(Max(maxValues.x, meshMaxValues.x), Max(maxValues.y, meshMaxValues.y), Max(maxValues.z, meshMaxValues.z));
				minValues = float3(Min(minValues.x, meshMinValues.x), Min(minValues.y, meshMinValues.y), Min(minValues.z, meshMinValues.z));
			}*/
			//newMesh->SetBasicModelMatrix(modelMatrix);
            newMesh->SetMaterialIndex(primitive.material);
			
			indexCount += newMesh->GetIndexCount();
			meshes.push_back(newMesh);
		}
	}

	// If this node contains children, send Load function to each one with current modelMatrix
	if (currentNode.children.size() > 0)
	{
		for (int i = 0; i < currentNode.children.size(); ++i) LoadRecursive(sourceModel, modelMatrix, currentNode.children[i]);
	}

	return;
}

void EngineModel::ClearVectors()
{
	for (auto it : meshes)
	{
		delete it;
	}

	for (auto it : materials)
	{
		it->FreeMaterials();
	}

	meshes.clear();
    materials.clear();
	
	firstMesh = true;
	indexCount = 0;
	renderTexture = -1;
}

ComponentMaterial& EngineModel::GetMaterial(const int index) {
	if (index >= 0 && index < materials.size())
    {
		return *materials[index];
	}
}
