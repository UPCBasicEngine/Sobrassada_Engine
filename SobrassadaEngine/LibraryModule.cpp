#include "LibraryModule.h"

#include "FileSystem.h"
#include "Globals.h"
#include "SceneImporter.h"

#include "document.h"
#include "prettywriter.h"
#include "stringbuffer.h"

LibraryModule::LibraryModule() {}

LibraryModule::~LibraryModule() {}

bool LibraryModule::Init()
{
    SceneImporter::CreateLibraryDirectories();

    return true;
}

bool LibraryModule::SaveScene(const char *path) const
{
    // Create doc JSON
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    rapidjson::Value scene(rapidjson::kObjectType);

    // Scene values
    uint64_t uid            = 0;
    std::string name        = "Test Scene";
    uint64_t rootGameObject = 0;

    // Create structure
    scene.AddMember("UID", uid, allocator);
    scene.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);
    scene.AddMember("RootGameObject", rootGameObject, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjects(rapidjson::kArrayType);

    // 1 gameobject
    rapidjson::Value A(rapidjson::kObjectType);
    A.AddMember("UID", 1001, allocator);
    A.AddMember("ParentUID", 987654321, allocator);

    // Child UUIDs
    rapidjson::Value childUIDsGO(rapidjson::kArrayType);
    childUIDsGO.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    A.AddMember("ChildUIDS", childUIDsGO, allocator);
    A.AddMember("RootComponentUID", 2001, allocator);

    gameObjects.PushBack(A, allocator);

    // Add gameObjects to scene
    scene.AddMember("GameObjects", gameObjects, allocator);

    // Serialize Components
    rapidjson::Value components(rapidjson::kArrayType);

    // 1 component
    rapidjson::Value B(rapidjson::kObjectType);

    B.AddMember("UID", 2001, allocator);
    B.AddMember("ParentUID", 1001, allocator);

    // Child UUIDs
    rapidjson::Value childUIDsComp(rapidjson::kArrayType);
    childUIDsComp.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    B.AddMember("ChildUIDS", childUIDsComp, allocator);

    B.AddMember("Type", "MeshRenderer", allocator);

    // Dependent
    B.AddMember("MeshUID", 3001, allocator);
    B.AddMember("MaterialUID", 4001, allocator);

    components.PushBack(B, allocator);

    // Add components to scene
    scene.AddMember("Components", components, allocator);

    doc.AddMember("Scene", scene, allocator);

    // Save file as JSON
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string fileName      = FileSystem::GetFileNameWithoutExtension(path);

    unsigned int bytesWritten = FileSystem::Save(path, buffer.GetString(), buffer.GetSize(), false);
    if (bytesWritten == 0)
    {
        GLOG("Failed to save scene file: %s", path);
        return false;
    }

    GLOG("%s saved as scene", fileName.c_str());

    return true;
}

bool LibraryModule::LoadScene(const char *path)
{
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON(path, doc);

    if (!loaded)
    {
        GLOG("Failed to load scene file: %s", path);
        return false;
    }

    if (!doc.HasMember("Scene") || !doc["Scene"].IsObject())
    {
        GLOG("Invalid scene format: %s", path);
        return false;
    }

    rapidjson::Value &scene = doc["Scene"];

    // Scene values
    uint64_t uid            = scene["UID"].GetUint64();
    std::string name        = scene["Name"].GetString();
    uint64_t rootGameObject = scene["RootGameObject"].GetUint64();

    // Deserialize GameObjects
    if (scene.HasMember("GameObjects") && scene["GameObjects"].IsArray())
    {
        const rapidjson::Value &gameObjects = scene["GameObjects"];
        for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
        {
            const rapidjson::Value &gameObject = gameObjects[i];

            uint64_t uidGO                     = gameObject["UID"].GetUint64();
            uint64_t parentuid                 = gameObject["ParentUID"].GetUint64();

            if (gameObject.HasMember("ChildUIDS") && gameObject["ChildUIDS"].IsArray())
            {
                const rapidjson::Value &childUIDs = gameObject["ChildUIDS"];
                for (rapidjson::SizeType j = 0; j < childUIDs.Size(); j++)
                {
                    uint64_t childUID = childUIDs[j].GetUint64();
                }
            }
            uint64_t rootComponentUUID = gameObject["RootComponentUID"].GetUint64();
        }
    }

    // Deserialize Components
    if (scene.HasMember("Components") && scene["Components"].IsArray())
    {
        const rapidjson::Value &components = scene["Components"];

        for (rapidjson::SizeType i = 0; i < components.Size(); i++)
        {
            const rapidjson::Value &component = components[i];

            uint64_t compUID                  = component["UID"].GetUint64();
            uint64_t parentUID                = component["ParentUID"].GetUint64();
            std::string type                  = component["Type"].GetString();

            // dependent
        }
    }

    GLOG("Scene loaded successfully: %s", name.c_str());
    return true;
}


void LibraryModule::AddTexture(const std::string &imageName, const std::string &ddsPath)
{
    textureMap[imageName] = ddsPath; // Map the texture name to its DDS path
}

std::string LibraryModule::GetTextureDDSPath(const std::string &imageName) const
{
    auto it = textureMap.find(imageName);
    if (it != textureMap.end())
    {
        return it->second; // Return the DDS path if the texture is found
    }
    return ""; // Return an empty string if texture is not found
}