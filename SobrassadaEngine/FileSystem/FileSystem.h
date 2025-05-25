#pragma once

#include "Globals.h"

#include "Libs/rapidjson/document.h"
#include <filesystem>
#include <string>

namespace FileSystem
{
    unsigned int Load(const char* filePath, char** buffer, bool asBinary = true);
    unsigned int LoadForDetour(const char* filePath, char** buffer);
    bool SOBRASADA_API_ENGINE LoadJSON(const char* scenePath, rapidjson::Document& doc);
    unsigned int Save(const char* filePath, const void* buffer, unsigned int size, bool asBinary = true, bool append = false);
    bool Copy(const char* sourceFilePath, const char* destinationFilePath);

    void GetDrives(std::vector<std::string>& drives);
    void SplitAccumulatedPath(const std::string& path, std::vector<std::string>& accPaths);
    void GetAllInDirectory(const std::string& path, std::vector<std::string>& files);
    void GetFilesSorted(const std::string& currentPath, std::vector<std::string>& files);
    time_t GetLastModifiedTime(const std::string& path);

    void AddDelimiterIfNotPresent(std::string& path);
    void RemoveDelimiterIfPresent(std::string& path);

    inline size_t GetFileSize(const char* filePath)
    {
        return std::filesystem::file_size(filePath);
    }

    inline bool Delete(const char* filePath)
    {
        return std::filesystem::remove(filePath);
    }

    inline bool CreateDirectories(const char* directoryPath)
    {
        return std::filesystem::create_directories(directoryPath);
    }

    inline bool Exists(const char* filePath)
    {
        return std::filesystem::exists(filePath);
    }

    inline bool IsDirectory(const char* directoryPath)
    {
        return std::filesystem::is_directory(directoryPath);
    }

    inline std::string GetFilePath(const std::string& filePath)
    {
        return std::filesystem::path(filePath).parent_path().string() + DELIMITER;
    }

    inline std::string GetParentPath(const std::string& filePath)
    {
        return std::filesystem::path(filePath).parent_path().string();
    }

    inline std::string GetFileNameWithExtension(const std::string& filePath)
    {
        return std::filesystem::path(filePath).filename().string();
    }

    inline std::string GetFileNameWithoutExtension(const std::string& filePath)
    {
        return std::filesystem::path(filePath).stem().string();
    }

    inline std::string GetFileExtension(const std::string& filePath)
    {
        return std::filesystem::path(filePath).extension().string();
    }

    inline bool IsAbsolute(const char* filePath)
    {
        return std::filesystem::path(filePath).is_absolute();
    }
} // namespace FileSystem