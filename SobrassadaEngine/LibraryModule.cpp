#include "LibraryModule.h"

#include "FileSystem.h"
#include "Globals.h"
#include "SceneImporter.h"


#include "document.h"
#include "stringbuffer.h"
#include "writer.h"
#include "prettywriter.h"
 textureMap[imageName] = ddsPath; // Map the texture name to its DDS path

LibraryModule::LibraryModule() {}

LibraryModule::~LibraryModule() {}

bool LibraryModule::Init()
{
    SceneImporter::CreateLibraryDirectories();
    std::string fileName = std::string("test.scene");
    std::string scene    = SCENES_PATH + fileName;
    SaveScene(scene.c_str());

    return true;
}

bool LibraryModule::SaveScene(const char *path)
{
    // Create doc JSON
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    rapidjson::Value scene(rapidjson::kObjectType);

    // Scene values
    uint64_t uuid                                 = 0;
    std::string name                              = "Test Scene";
    uint64_t rootGameObject                       = 0;

    // Create structure
    scene.AddMember("uuid", uuid, allocator);
    scene.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
    scene.AddMember("rootGameObject", rootGameObject, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjects(rapidjson::kArrayType);

    // 1 gameobject
    rapidjson::Value A(rapidjson::kObjectType);
    A.AddMember("uuid", 1001, allocator);
    A.AddMember("parentuuid", 987654321, allocator);

    // Child UUIDs
    rapidjson::Value childUUIDs(rapidjson::kArrayType);
    childUUIDs.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    A.AddMember("childuuids", childUUIDs, allocator);
    A.AddMember("rootComponentUUID", 2001, allocator);

    gameObjects.PushBack(A, allocator);

    // Add gameObjects to doc
    scene.AddMember("gameObjects", gameObjects, allocator);

    doc.AddMember("Scene", scene, allocator);

    // Save file as JSON
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string fileName      = FileSystem::GetFileNameWithoutExtension(path);

    unsigned int bytesWritten = FileSystem::Save(path, buffer.GetString(), buffer.GetSize(), false);
//main purpouse is to return the dds path using the texture name (So that we can store pointers to the dds files on material)
std::string LibraryModule::GetTextureDDSPath(const std::string &imageName) const { 
	
	auto it = textureMap.find(imageName);
    if (it != textureMap.end())
    if (bytesWritten == 0)
    {
        return it->second; // Return the DDS path if the texture is found
        GLOG("Failed to save scene file: %s", path);
        return false;
    }
    return ""; // Return an empty string if texture is not found

    GLOG("%s saved as scene", fileName.c_str());

    return true;
}
void LibraryModule::AddTexture(const std::string& imageName, const std::string& ddsPath) {

bool LibraryModule::LoadScene(const char *path) { return true; }
