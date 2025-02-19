#include "ResourceAnimation.h"

#include "Application.h"
#include "DirectXTex/DirectXTex.h"
#include "LibraryModule.h"
#include "TextureImporter.h"
#include "imgui.h"

#include <glew.h>
#include <unordered_set>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

ResourceAnimation::ResourceAnimation(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Material)
{
}

ResourceAnimation::~ResourceAnimation()
{
   
}
