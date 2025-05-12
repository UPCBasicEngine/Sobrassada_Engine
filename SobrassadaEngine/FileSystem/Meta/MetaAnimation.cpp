#include "MetaAnimation.h"
MetaAnimation::MetaAnimation(UID uid, const std::string& assetPath, int32_t index) : MetaFile(uid, assetPath)
{
    animationIndex = index;
}

void MetaAnimation::AddImportOptions(rapidjson::Document& doc, rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value importOptions(rapidjson::kObjectType);

    importOptions.AddMember("animationIndex", animationIndex, allocator);
    doc.AddMember("importOptions", importOptions, allocator);
}
