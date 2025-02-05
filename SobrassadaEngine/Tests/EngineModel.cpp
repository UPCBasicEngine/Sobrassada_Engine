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

void EngineModel::LoadTexture(const tinygltf::Model sourceModel, int textureIndex, ComponentMaterial* material, const char* modelPath) {
	const tinygltf::Texture &texture = sourceModel.textures[textureIndex];
    const tinygltf::Image &image     = sourceModel.images[texture.source];
    std::unordered_set<int> loadedIndices;
	float2 widthHeight     = float2::zero;
                    
    if (loadedIndices.find(texture.source) == loadedIndices.end())
    {
        std::string filePath     = std::string(modelPath);
        char usedSeparator       = '\\';

        int fileLocationPosition = (int)filePath.find_last_of(usedSeparator);
        if (fileLocationPosition == -1)
        {
            usedSeparator        = '/';
            fileLocationPosition = filePath.find_last_of(usedSeparator);
        }
                        
        if (fileLocationPosition == -1) return;

        std::string fileLocation      = filePath.substr(0, fileLocationPosition) + usedSeparator;
        std::string texturePathString = fileLocation.append(image.uri);

        std::wstring wideUri       = std::wstring(texturePathString.begin(), texturePathString.end());
        const wchar_t *texturePath = wideUri.c_str();

        DirectX::TexMetadata textureMetadata;
        unsigned int textureId = App->GetTextureModuleTest()->LoadTexture(texturePath, textureMetadata);
        if (textureId)
        {
			widthHeight.x = textureMetadata.width;
			widthHeight.y = textureMetadata.height;
            loadedIndices.insert(texture.source);
            material->setSpecularID(textureId);
            material->setHasSpecularTexture(true);

            textures.push_back(textureId);
            textureInfo.push_back(widthHeight);
            renderTexture++;
        }
    }
}

void EngineModel::LoadMaterials(const tinygltf::Model& sourceModel, const char* modelPath)
{
    std::unordered_set<int> loadedIndices;
    materials.clear();
    int id = 0;

    for (const auto &srcMaterial : sourceModel.materials)
    {
        ComponentMaterial material;
        material.SetName(srcMaterial.name);
        unsigned int textureId = 0;
        
		auto it = srcMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
        if (it != srcMaterial.extensions.end())
        {
            const tinygltf::Value &ext = it->second;

            if (ext.Has("diffuseFactor"))
            {
                const tinygltf::Value &diffuseValue = ext.Get("diffuseFactor");
                if (diffuseValue.IsArray() && diffuseValue.ArrayLen() == 4)
                {
                    material.SetDiffuseColor(
                    {
                            static_cast<float>(diffuseValue.Get(0).Get<double>()),
                            static_cast<float>(diffuseValue.Get(1).Get<double>()),
                            static_cast<float>(diffuseValue.Get(2).Get<double>()),
                    });
                }
            }

            if (ext.Has("specularFactor"))
            {
                const tinygltf::Value &specularValue = ext.Get("specularFactor");
                if (specularValue.IsArray() && specularValue.ArrayLen() == 3)
                {
                    material.setSpecularColor(
                    {
                        static_cast<float>(specularValue.Get(0).Get<double>()),
                        static_cast<float>(specularValue.Get(1).Get<double>()),
                        static_cast<float>(specularValue.Get(2).Get<double>())
                    });
                }
            }

            if (ext.Has("glossinessFactor"))
            {
                material.setShininess(ext.Get("glossinessFactor").Get<double>());
                material.setHasShininessInAlpha(false);
            }

            if (ext.Has("specularGlossinessTexture"))
            {
                int textureIndex = ext.Get("specularGlossinessTexture").Get("index").Get<int>();
                
                if (textureIndex >= 0)
                {
					material.setHasSpecularTexture(true);
                    LoadTexture(sourceModel, textureIndex, &material, modelPath);
                }
            }

			if (ext.Has("diffuseTexture"))
            {
                int textureIndex = ext.Get("diffuseTexture").Get("index").Get<int>();
                
                if (textureIndex >= 0)
                {
					material.SetHasDiffuseTexture(true);
                    LoadTexture(sourceModel, textureIndex, &material, modelPath);
                }
            }
        }

        //House
        else
        {  
            int textureIndex       = srcMaterial.pbrMetallicRoughness.baseColorTexture.index;
            if (textureIndex < 0)
            {
                material.SetDiffuseColor(
                {
                    static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[0]),
                    static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[1]),
                    static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[2])
                });
            }
            else
            {
                material.SetHasDiffuseTexture(true);
                LoadTexture(sourceModel, textureIndex, &material, modelPath);
            }
        }
        
        materials.push_back(material);
        id++;
    }
}

void EngineModel::LoadAdditionalTexture(const char* texturePath)
{
	std::string stringPath = std::string(texturePath);
	std::wstring widePath = std::wstring(stringPath.begin(), stringPath.end());
	const wchar_t* wideTexturePath = widePath.c_str();

	float2 widthHeight = float2::zero;

	DirectX::TexMetadata textureMetadata;
	unsigned int textureId = App->GetTextureModuleTest()->LoadTexture(wideTexturePath, textureMetadata);

	if (textureId) 
	{
		widthHeight.x = textureMetadata.width;
		widthHeight.y = textureMetadata.height;

		renderTexture++;
		textures.push_back(textureId);
		textureInfo.push_back(widthHeight);
	}
}

void EngineModel::Render(int program, float4x4& projectionMatrix, float4x4& viewMatrix)
{
	for (EngineMesh* currentMesh : meshes)
	{
        std::vector<int>& indices = currentMesh->GetMaterialIndices();
		int texturePosition = textures.size() > 0 ? renderTexture > -1 ? textures[renderTexture] : textures[textures.size() - 1] : 0;
		currentMesh->Render(program, texturePosition, projectionMatrix, viewMatrix);
	}
}

void EngineModel::GetTextureSize(float2& outTextureSize)
{
	if (renderTexture > -1)
	{
		outTextureSize.x = textureInfo[renderTexture].x;
		outTextureSize.y = textureInfo[renderTexture].y;
	}
}

void EngineModel::SetRenderTexture(int texturePosition)
{
	renderTexture = texturePosition;
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

			float3 meshMaxValues = modelMatrix.MulPos(newMesh->GetMaximumPosition());
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
			}

			newMesh->SetBasicModelMatrix(modelMatrix);
			
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

	for (unsigned int textureId : textures)
	{
		glDeleteTextures(1, &textureId);
	}

	meshes.clear();
	textures.clear();
	textureInfo.clear();
	
	firstMesh = true;
	indexCount = 0;
	renderTexture = -1;
}

ComponentMaterial &EngineModel::GetMaterial(const int index){
    if (index >= 0 && index < materials.size()) {
        return materials[index];
    }
}
