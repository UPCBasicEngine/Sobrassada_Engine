#include "MetaFile.h"

#include "Application.h"
#include "FileSystem.h"
#include "Globals.h"
#include "ProjectModule.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

MetaFile::MetaFile(UID uid, const std::string& assetPath)
    : assetUID(uid), lastModified(FileSystem::GetLastModifiedTime(assetPath))
{
}

void MetaFile::Save(const std::string& name, const std::string& assetPath) const
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    rapidjson::Value nameValue;
    nameValue.SetString(name.c_str(), static_cast<rapidjson::SizeType>(name.length()), allocator);

    rapidjson::Value pathValue;
    pathValue.SetString(assetPath.c_str(), static_cast<rapidjson::SizeType>(assetPath.length()), allocator);

    // Common metadata fields
    doc.AddMember("UID", assetUID, allocator);
    doc.AddMember("name", nameValue, allocator);
    doc.AddMember("assetPath", pathValue, allocator);
    doc.AddMember("lastModifiedDate", static_cast<UID>(lastModified), allocator);
    AddImportOptions(doc, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    UID prefix           = assetUID / UID_PREFIX_DIVISOR;
    std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH + std::to_string(prefix) +
                           FILENAME_SEPARATOR + name + META_EXTENSION;
    unsigned int bytesWritten =
        (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);

    if (bytesWritten == 0) GLOG("Failed to save meta file: %s", assetPath.c_str());

    savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_LIB_PATH + std::to_string(prefix) +
               FILENAME_SEPARATOR + name + META_EXTENSION;
    bytesWritten =
        (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);

    if (bytesWritten == 0) GLOG("Failed to save meta file: %s", assetPath.c_str());
}
